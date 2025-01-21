## **Prerequisites**

**Mosquitto Server/Client**
  ```
  $ sudo apt update
  $ sudo add-apt-repository ppa:mosquitto-dev/mosquitto-ppa
  $ sudo apt install mosquitto mosquitto-clients
   
  $ sudo systemctl status mosquitto
  ● mosquitto.service - Mosquitto MQTT Broker
       Loaded: loaded (/lib/systemd/system/mosquitto.service; enabled; vendor preset: enabled)
       Active: active (running) since Wed 2024-09-11 15:49:49 KST; 18min ago
         Docs: man:mosquitto.conf(5)
               man:mosquitto(8)
     Main PID: 19577 (mosquitto)
        Tasks: 1 (limit: 76827)
       Memory: 904.0K
       CGroup: /system.slice/mosquitto.service
               └─19577 /usr/sbin/mosquitto -c /etc/mosquitto/mosquitto.conf
  ```

**Mosquitto service config**
  ```
  $ vi /etc/mosquitto/mosquitto.conf
  ...
  ### Port to use for the default listener
  ### standard unencrypted MQTT port
  #listener 1883
  
  ### certificate based SSL/TLS encryption
  listener 8883
  
  ### the PEM encoded Certificate Authority certificates
  cafile <path to CA certificate file>
  
  ### the path to a file containing the CA certificates, 'openssl rehash <path to capath>' each time   you add/remove a certificate.
  #capath <path to directroy containing CA certificates>
  
  ### Path to the PEM encoded server certificate.
  certfile <path to certificate file>
  
  ### Path to the PEM encoded server keyfile.
  keyfile <path to key file>
  
  ### user:password, mosquitto_passwd is a tool for managing password files
  #password_file /etc/mosquitto/passwd
  
  ### Authenticated access
  #allow_anonymous false
  
  ### Unauthenticated access
  allow_anonymous true
  ```

## **Simple Publisher/Subscriber Client**

- **Subscriber**
  ```
  $ mosquitto_sub -h localhost -t myTopic
  ```   

- **Publisher**
  ```
  $ mosquitto_pub -h localhost -t myTopic -m "Hello 1"
  ```   

## **Secure connection**

- **CA certificates / Server keys, certificates**
  ```
  ** Test Root CA
  $ openssl genrsa -out ca.key 2048
  $ openssl req -new -x509 -days 360 -key ca.key -out ca.crt -subj "/C=KR/ST=KK/L=SN/O=DXS/OU=Test/  CN=TestCA"
  
  ** Server Key and Certificate(Server's CN must match the host)
  $ openssl genrsa -out server.key 2048
  $ openssl req -new -out server.csr -key server.key -subj "/C=KR/ST=KK/L=SN/O=DXS/OU=Server/  CN=DXS-BROKER"
  $ openssl x509 -req -in server.csr -CA ca.crt -CAkey ca.key -CAcreateserial -out server.crt -days 360
  $ openssl verify -CAfile ca.crt server.crt
  
  $ ls -al
  ca.crt  ca.key  ca.srl  server.crt  server.csr  server.key
  
  $ cat /etc/mosquitto/mosquitto.conf
  listener 8883
  cafile /etc/mosquitto/ca_certificates/ca.crt
  certfile /etc/mosquitto/certs/server.crt
  keyfile /etc/mosquitto/certs/server.key
  ...
  ```

- **Id/Password**
  ```
  $ sudo mosquitto_passwd -c /etc/mosquitto/passwd user        ##create passwd file and add id
  Password:
  Reenter password:
  $ sudo mosquitto_passwd -b /etc/mosquitto/passwd user1 1234  ##add id
  $ sudo mosquitto_passwd -D /etc/mosquitto/passwd user1       ##remove id
  $ sudo chmod 644 /etc/mosquitto/passwd
  
  $ cat /etc/mosquitto/mosquitto.conf 
  password_file /etc/mosquitto/passwd
  allow_anonymous false
  ...
  ```