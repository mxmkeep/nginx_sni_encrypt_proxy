# nginx_sni_encrypt_proxy
Use local Nginx to encrypt the TLS SNI field, send it to remote Nginx for SNI decryption, and then forward it to achieve the forwarding of specific HTTPS traffic.

![image](https://github.com/mxmkeep/nginx_sni_encrypt_proxy/assets/20048552/d4b5492a-6681-4fdc-ba89-89a99b68919a)

## How does it work
Modiyfied ngx_stream_ssl_preread_module, add three directives 
* ssl_sni_encrypt: on/off(default), encrypt https's sni by rijndael witch cbc.
* ssl_sni_decrypt: on/off(default), decrypt https's sni by rijndael witch cbc.
* key_base64: the key of encrypt or decrypt.
The rijndael code file is from project fwknop[https://github.com/mrash/fwknop]

## Why need ngx_http_proxy_connect_module
The project can still be run without ngx_http_proxy_connect_module. You need modify local hosts file, but you will encounter two issues.
1. The hosts file does not support pan-domain configuration, you need to find the sub-domain to configure, it is very troublesome, especially like youbute has random sub-domains, it is impossible to manually change the hosts file! Of course, you can also setup your own DNS service, but the following issue are more critical.
2. Multiplexing of http 2.0: Because we point all traffic to the same IP for proxying, if the target domain supports http2.0, then subdomains with different source IPs will also use the same tcp socket for transmission, which will cause problems. The http connect method will let the browse create new sockets for every session.

## Install
Base on nginx-1.24.0

```shell
# on ubuntu 22
#download nginx
apt update
apt install -y libpcre3 libpcre3-dev libssl-dev openssl 
wget https://nginx.org/download/nginx-1.24.0.tar.gz
tar -zxvf nginx-1.24.0.tar.gz

#add ngx_http_proxy_connect_module
git clone https://github.com/chobits/ngx_http_proxy_connect_module.git
mv ngx_http_proxy_connect_module nginx-1.24.0/src/
cd nginx-1.124.0/
patch -p1 < src/ngx_http_proxy_connect_module/patch/proxy_connect_rewrite_102101.patch

#Embed this project's code
mv nginx-1.24.0/src/stream/ngx_stream_ssl_preread_module.c nginx-1.24.0/src/stream/ngx_stream_ssl_preread_module.c.bak
git clone https://github.com/mxmkeep/nginx_sni_encrypt_proxy.git
cp ngx_stream_ssl_preread_module.c nginx-1.24.0/src/stream/
cp rijndael.c nginx-1.24.0/src/stream/
cp rijndael.h nginx-1.24.0/src/stream/

vim auto/modules and find out ngx_stream_ssl_preread_module, add rijndael file
    if [ $STREAM_SSL_PREREAD = YES ]; then
        ngx_module_name=ngx_stream_ssl_preread_module
        ngx_module_deps=src/stream/rijndael.h
        ngx_module_srcs="src/stream/ngx_stream_ssl_preread_module.c \
                        src/stream/rijndael.c"
        ngx_module_libs=
        ngx_module_link=$STREAM_SSL_PREREAD

#build
./configure  \
--prefix=/usr/local/nginx \
--error-log-path=/home/nginx/log/error.log \
--http-log-path=/home/nginx/log/access.log \
--pid-path=/home/nginx/run/nginx.pid \
--lock-path=/home/nginx/run/nginx.lock \
--user=nginx \
--group=nginx \
--with-http_ssl_module \
--add-module=src/ngx_http_proxy_connect_module \
--with-stream \
--with-stream_ssl_module \
--with-stream_ssl_preread_module

make -j 4
make install

mkdir -p /home/nginx/log/
mkdir -p /usr/local/nginx/client_body_temp
mkdir -p /home/nginx/run

cp proxy.pac /usr/local/nginx/html/
cp nginx.conf /usr/local/nginx/conf/

#gen your own key and replace key_base64 in nginx.conf
#a key with random 32 bytes and encode to base64
python3 genkey.py

```

### local nginx conf
```shell
#user  nobody;
user  root;
worker_processes  auto;

#daemon off;
#master_process off;

#error_log  logs/error.log;
#error_log  logs/error.log  notice;
#error_log  logs/error.log  info;

#pid        logs/nginx.pid;


events {
    worker_connections  1024;
}


http {
    include       mime.types;
    default_type  application/octet-stream;

    #log_format  main  '$remote_addr - $remote_user [$time_local] "$request" '
    #                  '$status $body_bytes_sent "$http_referer" '
    #                  '"$http_user_agent" "$http_x_forwarded_for"';

    #access_log  logs/access.log  main;

    sendfile        on;
    #tcp_nopush     on;

    #keepalive_timeout  0;
    keepalive_timeout  65;

    #gzip  on;

    #return proxy.pac file to wifi
    server {
        listen 8080;
        server_name proxy_file;
        location = /proxy.pac {
            root html;
        }
    }

    # Only support https
    server {
        listen       80;
        server_name  localhost;
		#redirect to https
        rewrite ^(.*)$ https://$host$1  permanent;
    }

    #proxy connect service
    server {
        listen                         3128;

        # dns resolver used by forward proxying
        #resolver                       8.8.8.8;
	    
        # forward proxy for CONNECT requests
        proxy_connect;
        proxy_connect_address 127.0.0.1;  #connect to local stream module
        proxy_connect_allow            443 563;
        proxy_connect_connect_timeout  10s;
        proxy_connect_data_timeout     10s;
	    
        # defined by yourself for non-CONNECT requests
        # Example: reverse proxy for non-CONNECT requests
        location / {
            proxy_pass http://$host;
            proxy_set_header Host $host;
        }
    }
}


stream{

    log_format basic ' [$time_local] $remote_addr $ssl_preread_server_name $upstream_addr '
                 '$ssl_preread_protocol $ssl_preread_alpn_protocols '
                 '$protocol $status $bytes_sent $bytes_received '
                 '$session_time';
    access_log /home/nginx/log/stream-access.log basic buffer=32k  flush=5m;

    #redirect different domain to different upstream
    map $ssl_preread_server_name $name {
        hostnames;
        .github.com          node_sin;
        .openai.com          node_tyo;
        default             node_hkg;
    }

    upstream node_hkg {
        server 1.1.1.1:11443;
    }

    upstream node_tyo {
        server 2.2.2.2:11443;
    }

    upstream node_sin {
        server 3.3.3.3:11443;
    }

    #local https service
    #must 443 port, because browse connect to 443
    #you can with assign different listen IPs to fulfill multi needs
    server{
        listen 443 reuseport;
        proxy_pass $name ;
        ssl_preread on;
        ssl_sni_encrypt on;
        key_base64 0PSgMqo/B/5WdQsIcA+bEjYWzQrXPUtJcrrcpSXZXyk=;
    }
}
```

### remote nginx conf
```shell
#user  nobody;
user  root;
worker_processes  auto;

#daemon off;
#master_process off;

#error_log  logs/error.log;
#error_log  logs/error.log  notice;
#error_log  logs/error.log  info;

#pid        logs/nginx.pid;


events {
    worker_connections  1024;
}

stream{

    log_format basic ' [$time_local] $remote_addr $ssl_preread_server_name $upstream_addr '
                 '$ssl_preread_protocol $ssl_preread_alpn_protocols '
                 '$protocol $status $bytes_sent $bytes_received '
                 '$session_time';
    access_log /home/nginx/log/stream-access.log basic buffer=32k  flush=5m;

    #remote https service
    server{
        listen 11443 reuseport;
        resolver 8.8.8.8 114.114.114.114 valid=2m; #modify to your remote server's DNS IP
        proxy_pass $ssl_preread_server_name:443;
        ssl_preread on;
        ssl_sni_decrypt on;
        key_base64 0PSgMqo/B/5WdQsIcA+bEjYWzQrXPUtJcrrcpSXZXyk=;
    }

}
```

### Start nginx
```/usr/local/nginx/sbin/nginx -c /usr/local/nginx/conf/nginx.conf```

## configure computer's proxy setting
![3bc6956b0fd84992f5fe94da411da99](https://github.com/mxmkeep/nginx_sni_encrypt_proxy/assets/20048552/3b25947e-6ebe-4945-8157-cca6a3895197)








