# nginx_sni_encrypt_proxy
Use local Nginx to encrypt the TLS SNI field, send it to remote Nginx for SNI decryption, and then forward it to achieve the forwarding of specific HTTPS traffic.

This is the first version, and I will update the README to explain what's going on and how to use it.

base on nginx-1.24.0
 
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

#
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


#start nginx
/usr/local/nginx/sbin/nginx -c /usr/local/nginx/conf/nginx.conf


```


![3bc6956b0fd84992f5fe94da411da99](https://github.com/mxmkeep/nginx_sni_encrypt_proxy/assets/20048552/3b25947e-6ebe-4945-8157-cca6a3895197)






