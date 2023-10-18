import secrets
import base64

# 生成32字节的随机密钥
random_key = secrets.token_bytes(32)

# 使用Base64进行编码
base64_key = base64.b64encode(random_key).decode('utf-8')

print("KeyGen(32)with base64:", base64_key)


