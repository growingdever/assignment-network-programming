from socket import *
import sys

HOST = '127.0.0.1'
PORT = 8888
MAX_LENGTH = 4096
ADDR = (HOST, PORT)

def until_crlf(line):
	return line.split('\r')[0]

sock_client = socket(AF_INET, SOCK_STREAM)
try:
	sock_client.connect(ADDR)
except Exception as e:
	print e
	sys.exit()

sock_client.send('PWD\r\n')
data = sock_client.recv(MAX_LENGTH)
data = until_crlf(data)
print data
if data != '/Users/loki/programming/assignment-network-programming/ftpserver':
	print 'test fail'
	sys.exit()
else:
	print 'pass 1'


sock_client.send('MKD /test\r\n')
data = sock_client.recv(MAX_LENGTH)
data = until_crlf(data)
print data
if data != 'success':
	print 'test fail'
	sys.exit()
else:
	print 'pass 2'


sock_client.send('NLST\r\n')
data = sock_client.recv(MAX_LENGTH)
data = until_crlf(data)
print data
if data != '., .., build, test':
	print 'test fail'
	sys.exit()
else:
	print 'pass 3'


sock_client.send('CWD /Users/loki/programming/assignment-network-programming/ftpserver/test\r\n')
data = sock_client.recv(MAX_LENGTH)
data = until_crlf(data)
print data
if data != 'success':
	print 'test fail'
	sys.exit()
else:
	print 'pass 4'


sock_client.close()



# echo -n "CWD /test\r\n" | nc 127.0.0.1 21;
# echo -n "STOR ~/Documents/jess_in_action_ebook.pdf\r\n" | nc 127.0.0.1 21;
# echo -n "RETR /test/jess_in_action_ebook.pdf\r\n" | nc 127.0.0.1 21;
# echo -n "STOR ~/Documents/onemore\r\n" | nc 127.0.0.1 21;
# echo -n "LIST\r\n" | nc 127.0.0.1 21;
# echo -n "DELE onemore\r\n" | nc 127.0.0.1 21;
# echo -n "LIST\r\n" | nc 127.0.0.1 21;
# echo -n "CWD /test\r\n" | nc 127.0.0.1 21;
# echo -n "MKD /test2\r\n" | nc 127.0.0.1 21;
# echo -n "NLST\r\n" | nc 127.0.0.1 21;
# echo -n "RMD /test\r\n" | nc 127.0.0.1 21;
# echo -n "RMD /test2\r\n" | nc 127.0.0.1 21;
# echo -n "QUIT\r\n" | nc 127.0.0.1 21;
