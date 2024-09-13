from threading import Thread
from multiprocessing import Process
import time


def create_pubs(id_):
    '''creates publishers at isolated discovery network, isolated network is created based on id_'''
    import os
    isolated_env = {'GZ_DISCOVERY_MSG_PORT':f'{11320+id_*10}',
            'GZ_DISCOVERY_SRV_PORT':f'{11321+id_*10}',
            'GZ_DISCOVERY_MULTICAST_IP': f'239.255.0.{10+id_}',
            'GZ_PARTITION':f'env{id_}',
            }
                        
    print("Exec the following in cli to listen to topics at isolated env", id_)
    [print(f'export {key}={isolated_env[key]}') for key in isolated_env]
    os.environ.update(isolated_env)
    import gz.transport14 as transport
    from gz.msgs11.empty_pb2 import Empty
    node = transport.Node()
    pub = node.advertise(f'/empty{id_}', Empty)
    import time 
    while True:
        print(f"publishing at env {id_}")
        pub.publish(Empty())
        time.sleep(1.0)
    


def pub_proc(n_threads):
    '''process for creating multiple pubs'''

    pub_threads =[Thread(target=create_pubs,args=[i]) for i in range(n_threads)]
    for pi in pub_threads:
        pi.start()
        time.sleep(2.0)
    for pi in pub_threads:
        pi.join()



proc_pub = Thread(target=pub_proc, args=[2])
proc_pub.start()
proc_pub.join()