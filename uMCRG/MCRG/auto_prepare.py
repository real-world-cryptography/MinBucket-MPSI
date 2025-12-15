from multiprocessing.connection import wait
import os
import pwd
import re
import threading
import _thread
import subprocess
import random
import string
import json
import time
import signal
import argparse


#make csv files for sender and receivers
sender_list = []
script_dir = os.path.dirname(os.path.abspath(__file__))
csv_folder = os.path.join(script_dir, "csvfiles")

def init_csv_folder():
    os.makedirs(csv_folder, exist_ok=True) 

def prepare_sender_data(sender_sz,item_bc): 
        
    folder_path = os.path.join(script_dir, 'csvfiles')

    sender_path = os.path.join(csv_folder, "sender.csv")
    label_bc = 0
    letters = string.ascii_lowercase + string.ascii_uppercase
    while len(sender_list) < sender_sz:
        item = ''.join(random.choice(letters) for i in range(item_bc))
        label = ''.join(random.choice(letters) for i in range(label_bc))
        sender_list.append((item, label))
    print('Done creating sender\'s set')
    
    
    with open(sender_path, "w") as sender_file:
        for (item, label) in sender_list:
            sender_file.write(item + (("," + label) if label_bc != 0 else '') + '\n')
    print('Wrote sender\'s set')
    
def prepare_receiver_data(recv_sz,int_sz,item_bc,num,flag):
    folder_path = os.path.join(os.path.dirname(__file__), 'csvfiles') 
    
    receiver_set_name='receiver_'+flag+'_'+str(num)+'.csv'
    receiver_path = os.path.join(csv_folder, receiver_set_name)

    letters = string.ascii_lowercase + string.ascii_uppercase
    recv_set = set()
    while len(recv_set) < min(int_sz, recv_sz):
        item = random.choice(sender_list)[0]
        recv_set.add(item)
        
    while len(recv_set) < recv_sz:
        item = ''.join(random.choice(letters) for i in range(item_bc))
        recv_set.add(item)
    print('Done creating receiver\'s set')
    
    with open(receiver_path, "w") as recv_file:
        for item in recv_set:
            recv_file.write(item + '\n')
    print('Wrote receiver\'s set    '+ receiver_set_name)


#receiver_hash
def run_command(command):
    try:
        subprocess.run(command, check=True, shell=True)
    except subprocess.CalledProcessError as e:
        print(f"An error occurred: {e}")
        
def receiver_hash(small_receiver_num):

   
    # Get the absolute path of the current script
    mcrg_dir = os.path.dirname(os.path.abspath(__file__))
    hash_dir = os.path.join(mcrg_dir, 'build')
    csv_dir = os.path.join(mcrg_dir, 'csvfiles')

    
     # Create a folder if it does not exist
    if not os.path.exists(os.path.join(mcrg_dir, 'hash_result')):
        os.makedirs(os.path.join(mcrg_dir, 'hash_result'))
    if not os.path.exists(os.path.join(mcrg_dir, 'mcrg_result')):
        os.makedirs(os.path.join(mcrg_dir, 'mcrg_result'))


    # Start receiver and sender in the background
    #./bin/receiver_cli_ddh -d db.csv -p ../parameters/16M-1024.json -t 1 --serial_number 0
 
    print("\n\n\nstart for receiver hash\n\n\n")
    for i in range(small_receiver_num):
        filename="receiver_1_"+str(i)+".csv"
        #filename = "sender.csv"
        receiver_command = f'{os.path.join(hash_dir, "bin", "simple_hash")} -d {os.path.join(csv_dir, filename)} -p {os.path.join(hash_dir, "16M-1024.json")}  -t 1 --serial_number {i} '
        run_command(receiver_command)
    print("\n\nend for receiver hash\n\n\n")

    
if __name__ =="__main__":

    
    parser = argparse.ArgumentParser(description='Script to automate the test process', formatter_class=argparse.RawTextHelpFormatter)
    parameter_group = parser.add_argument_group('Parameters', 'Configuration for protocol execution:')
    parameter_group.add_argument('-bn', type=int, required=True, help='the number of big set')
    parameter_group.add_argument('-sn', type=int, required=True, help='the number of small set')

    args = parser.parse_args()

    big_num=args.bn
    small_num=args.sn
    
    init_csv_folder()
    
    prepare_sender_data(pow(2,10),16)
    for i in range(small_num):
        prepare_receiver_data(pow(2,10),256,16,i,'1')
    for i in range(big_num):
        prepare_receiver_data(pow(2,14),256,16,i,'0')
    print('MAKECSV DONE!')
    receiver_hash(small_num)
    print('SIMPLE DONE!')

    
