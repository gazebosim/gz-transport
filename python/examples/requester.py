from gz.msgs10.stringmsg_pb2 import StringMsg
from gz.transport13 import Node

def main():
    node = Node()
    service_name = "/echo"
    request = StringMsg()
    request.data = "Hello world"
    response = StringMsg()
    timeout = 5000

    result, response = node.request(service_name, request, StringMsg, StringMsg, timeout, response)
    print("Result:", result, "\nResponse:", response.data)

if __name__ == "__main__":
    main()
