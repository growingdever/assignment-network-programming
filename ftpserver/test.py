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


filename = 'screenshot.png'
sock_client.send('STOR %s\r\n' % filename)
data = sock_client.recv(MAX_LENGTH)
data = until_crlf(data)
print data
if data.split()[0] != 'OK':
	print 'test fail'
	sys.exit()
else:
	new_port = data.split()[1]
	sock_client_data = socket(AF_INET, SOCK_STREAM)
	try:
		sock_client_data.connect((HOST, (int)(new_port)))
	except Exception as e:
		print e
		print 'test fail'
		sys.exit()

	with open('/Users/loki/Documents/%s' % filename, 'rb') as f:
		bytes_read = f.read(MAX_LENGTH)
		while bytes_read:
			for b in bytes_read:
				sock_client_data.send(b)
			bytes_read = f.read(MAX_LENGTH)
	sock_client_data.close();
	print 'pass 5'


sock_client.send('RETR %s\r\n' % filename)
data = sock_client.recv(MAX_LENGTH)
data = until_crlf(data)
print data
if data.split()[0] != 'OK':
	print 'test fail'
	sys.exit()
else:
	new_port = data.split()[1]
	sock_client_data = socket(AF_INET, SOCK_STREAM)
	try:
		sock_client_data.connect((HOST, (int)(new_port)))
	except Exception as e:
		print e
		print 'test fail'
		sys.exit()

	with open('%s' % filename, 'wb') as f:
		bytes_read = sock_client_data.recv(MAX_LENGTH)
		while bytes_read:
			for b in bytes_read:
				f.write(b)
			bytes_read = sock_client_data.recv(MAX_LENGTH)
	sock_client_data.close();
	print 'pass 6'


filename2 = 'hello.py'
sock_client.send('STOR %s\r\n' % filename2)
data = sock_client.recv(MAX_LENGTH)
data = until_crlf(data)
print data
if data.split()[0] != 'OK':
	print 'test fail'
	sys.exit()
else:
	new_port = data.split()[1]
	sock_client_data = socket(AF_INET, SOCK_STREAM)
	try:
		sock_client_data.connect((HOST, (int)(new_port)))
	except Exception as e:
		print e
		print 'test fail'
		sys.exit()

	with open('/Users/loki/Documents/%s' % filename2, 'rb') as f:
		bytes_read = f.read(MAX_LENGTH)
		while bytes_read:
			for b in bytes_read:
				sock_client_data.send(b)
			bytes_read = f.read(MAX_LENGTH)
	sock_client_data.close();
	print 'pass 7'


sock_client.send('LIST\r\n')
data = sock_client.recv(MAX_LENGTH)
data = until_crlf(data)
print data
if data == 'fail':
	print 'test fail'
	sys.exit()
else:
	print 'pass 8'


sock_client.send('DELE %s\r\n' % filename2)
data = sock_client.recv(MAX_LENGTH)
data = until_crlf(data)
print data
if data != 'success':
	print 'test fail'
	sys.exit()
else:
	print 'pass 9'


sock_client.send('LIST\r\n')
data = sock_client.recv(MAX_LENGTH)
data = until_crlf(data)
print data
if data == 'fail':
	print 'test fail'
	sys.exit()
else:
	print 'pass 10'


sock_client.send('CWD /Users/loki/programming/assignment-network-programming/ftpserver\r\n')
data = sock_client.recv(MAX_LENGTH)
data = until_crlf(data)
print data
if data != 'success':
	print 'test fail'
	sys.exit()
else:
	print 'pass 11'


sock_client.send('MKD /test2\r\n')
data = sock_client.recv(MAX_LENGTH)
data = until_crlf(data)
print data
if data != 'success':
	print 'test fail'
	sys.exit()
else:
	print 'pass 12'


sock_client.send('NLST\r\n')
data = sock_client.recv(MAX_LENGTH)
data = until_crlf(data)
print data
if data != '., .., build, test, test2':
	print 'test fail'
	sys.exit()
else:
	print 'pass 13'


sock_client.send('RMD /Users/loki/programming/assignment-network-programming/ftpserver/test2\r\n')
data = sock_client.recv(MAX_LENGTH)
data = until_crlf(data)
print data
if data != 'success':
	print 'test fail'
	sys.exit()
else:
	print 'pass 14'


sock_client.send('RMD /Users/loki/programming/assignment-network-programming/ftpserver/test\r\n')
data = sock_client.recv(MAX_LENGTH)
data = until_crlf(data)
print data
if 'fail' not in data:
	print 'test fail'
	sys.exit()
else:
	print 'pass 15'


sock_client.close()


# echo -n "RMD /test\r\n" | nc 127.0.0.1 21;
# echo -n "RMD /test2\r\n" | nc 127.0.0.1 21;
# echo -n "QUIT\r\n" | nc 127.0.0.1 21;
