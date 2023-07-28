
# TLS for kafka

__overview__

To deploy SSL, the general steps are:

1. Generate the keys and certificates
2. Create your own Certificate Authority (CA)
3. Sign the certificate

The steps to create keys and sign certificates are enumerated below. You may also adapt a script from [confluent-platform-security-tools.git](https://github.com/confluentinc/confluent-platform-security-tools/blob/master/kafka-generate-ssl.sh).

The definitions of the parameters used in the steps are as follows:

1. keystore: the location of the keystore
2. ca-cert: the certificate of the CA
3. ca-key: the private key of the CA
4. ca-password: the passphrase of the CA
5. cert-file: the exported, unsigned certificate of the server
6. cert-signed: the signed certificate of the server

--- 

# Quick steps

[reference](https://github.com/vinclv/data-engineering-minds-kafka/tree/main/config/ssl)


__create keys__

**NOTE**: ensure that the common name (CN) exactly matches the fully qualified domain name (FQDN) of the server.
The client compares the CN with the DNS domain name to ensure that it is indeed connecting to the desired server, not a malicious one.
The hostname of the server can also be specified in the Subject Alternative Name (SAN).
Since the distinguished name is used as the server principal when SSL is used as the interbroker security protocol, it is useful to have hostname as a SAN rather than the CN.


- create a certificate authority (create anywhere) and you will need to copy this to each node (broker, producer, consumer)
```bash
# create a file called openssl.cnf
$ cat openssl.cnf
[req]
distinguished_name = req_distinguished_name
req_extensions = v3_req
prompt = no

[req_distinguished_name]
C = CA
ST = BC
L = Kelowna
O = AlphaWise
OU = Engineering
CN = 192.168.1.73

[v3_req]
subjectAltName = IP:192.168.1.73

# create CA
openssl req -new -x509 -keyout ca-key -out ca-cert -days 3650 -config openssl.cnf -passout pass:1234
```

- copy ca-cert and ca-key to each node (broker, producer, consumer), then create the keys on each node

- ===> change `DOMAIN` to the IP address of the device (if using SAN=IP:$DOMAIN)
  _or_
- ===> change `DOMAIN` to the host name of the device (if using SAN=DNS:$DOMAIN)

```bash
export DOMAIN=192.168.1.73
export DOMAIN=172.17.0.2
# on broker
export NAME=broker0
# on producer
export NAME=producer
# on consumer 
export NAME=consumer

# general steps (update NAME for each location you install it {broker, producer, consumer}
# add a new password, and trust cert
keytool -keystore kafka.$NAME.truststore.jks -alias ca-cert -import -file ca-cert -storepass 123456 -noprompt 
# full name = $DOMAIN, fill in rest
keytool -keystore kafka.$NAME.keystore.jks -alias $NAME -validity 3650 -genkey -keyalg RSA -ext SAN=IP:$DOMAIN -storepass 123456 -keypass 123456 -dname "CN=${DOMAIN}, OU=Engineering, O=AlphaWise, L=Kelowna, ST=BC, C=CA"
keytool -keystore kafka.$NAME.keystore.jks -alias $NAME -certreq -file ca-request-$NAME -storepass 123456 -keypass 123456
# enter password to ca-cert
openssl x509 -req -CA ca-cert -CAkey ca-key -in ca-request-$NAME -out ca-signed-$NAME -days 3650 -CAcreateserial -passin pass:1234

# add password, trust certificate (may change -alias ca-cert)
keytool -keystore kafka.$NAME.keystore.jks -alias ca-cert -import -file ca-cert -storepass 123456 -keypass 123456 -noprompt 
# add password
keytool -keystore kafka.$NAME.keystore.jks -alias $NAME -import -file ca-signed-$NAME -storepass 123456 -keypass 123456 -noprompt

# start kafka server
/usr/bin/supervisord -c "/etc/supervisor/conf.d/supervisord.conf"

# add this file to node 2 (client_security.properties)
## NOTE: on local, we need to include ssl.endpoint.identification.algorithm=<empty> because we don't have domain names!
security.protocol=SSL
ssl.truststore.location=/tmp/certs/kafka.consumer.truststore.jks
ssl.truststore.password=123456
ssl.keystore.location=/tmp/certs/kafka.consumer.keystore.jks
ssl.keystore.password=123456
ssl.key.password=123456
ssl.endpoint.identification.algorithm=

# test connection on other node (not kafka server)
$ ./kafka-topics.sh --bootstrap-server 192.168.1.73:9093 --create --topic mytopic2 --partitions 2 --command-config $(pwd)/client_security.properties

# you can check your certificates on node 2
keytool -list -v -keystore kafka.$NAME.keystore.jks -storepass 123456
```

---


__broker__

- configure each "**broker**" with the following fields in `server.properties`

```bash
# Configure the password, truststore, and keystore
ssl.truststore.location=/var/ssl/private/kafka.server.truststore.jks
ssl.truststore.password=test1234
ssl.keystore.location=/var/ssl/private/kafka.server.keystore.jks
ssl.keystore.password=test1234
ssl.key.password=test1234
# configure communication strategy for external (SSL) and inter-broker (PLAINTEXT)
listeners=PLAINTEXT://kafka1:9092,SSL://kafka1:9093
advertised.listeners=PLAINTEXT://<localhost>:9092,SSL://<localhost>:9093

# optional settings
ssl.cipher.suites=[list, all enabled]
ssl.enabled.protocols=TLSv1.2,TLSv1.1,TLSv1
ssl.truststore.type=JKS
```

__client__

- If client authentication is not required by the broker, the following is a minimal configuration example that you can store in a client properties file `client-ssl.properties`
```bash
bootstrap.servers=kafka1:9093
security.protocol=SSL
ssl.truststore.location=/var/ssl/private/kafka.client.truststore.jks
ssl.truststore.password=test1234
```

Examples using kafka-console-producer and kafka-console-consumer, passing in the client-ssl.properties file with the properties defined above:

```bash
bin/kafka-console-producer --broker-list kafka1:9093 --topic test --producer.config client-ssl.properties
bin/kafka-console-consumer --bootstrap-server kafka1:9093 --topic test --consumer.config client-ssl.properties --from-beginning
```