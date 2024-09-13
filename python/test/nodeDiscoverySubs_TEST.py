from threading import Thread
from multiprocessing import Process
import time



def create_subs(id_):
    '''creates subscribers at isolated discovery network, isolated network is created based on id_'''
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
    def cb(msg):
        print(f"Recieved msg at env {id_}")

    sub = node.subscribe(Empty, f'/empty{id_}', cb)

    import time 
    while True:
        time.sleep(1.0)




def sub_proc(n_threads):
    '''process for creating multiple subs'''
    # We reverse the order of env, this is to ensure
    # that the mew environments do not keep the initial envf vars
    # For example, consider the scenario where
    # two publishers are created with ids 0,1
    # If the env var change did affect them, then each one should be publishing
    # with different discovery ports
    # If it didn't affect, then all of them should be publishing at same discovery port
    # In order to verify that these publishers are indeed publishing at different ports
    # We can reverse the order of env id, when creating subscribers.
    # This ensures that the env var change indeed affected
    # See below:
    # 1. pub1 and pub2 created under same discovery (order of env creation 0->1)
    #    sub1 and sub2 created under same discovery (order of env creation 0->1)
    #    Since they are under same discovery, both pubs and subs will receive their msgs
    #    But its not easy to verify if they were created under different discovery
    # 2. pub1 and pub2 created under different discovery (order of env creation 0->1)
    #    sub1 and sub2 created under different discovery (order of env creation 1->0)
    #    If they are under same discovery, both subs will NOT receive their msgs

    sub_threads =[Thread(target=create_subs,args=[i]) for i in reversed(range(n_threads))]
    for si in sub_threads:
        si.start()
        time.sleep(2.0)
    for si in sub_threads:
        si.join()

proc_sub = Thread(target=sub_proc, args=[2])
proc_sub.start()
proc_sub.join()