#!/usr/bin/python3

'''
Name: Alexander Miranda
Due Date: November 24, 2017
Program: Python Exploration
Course: CS 344 - Operating Systems
'''

import os
import string
import random
from functools import reduce

def generate_file_data(length=10):
    '''
    Generates a 10 char length of random chars and returns that string
    '''
    chars = string.ascii_lowercase
    file_data = ''.join(random.choice(chars) for i in range(length))

    return file_data

def make_file(num=3):
    '''
    Generates a text file with a random 10 char filename and a 10 char string
    as its contents with a newline character
    '''
    for i in range(0, num):
        f_ptr = open(generate_file_data(), 'w+')
        content = generate_file_data()
        print(content)
        f_ptr.write(content + '\n')
        f_ptr.close()

def random_num_gen(int_count=2, min=1, max=42):
    '''
    Generates random numbers and outputs them to the console
    also will output the product of those randomly generated numbers
    '''
    int_arr = []
    for i in range(0, int_count):
        num = random.randint(min, max)
        print(num)
        int_arr.append(num)
    print(reduce(lambda x, y: x * y, int_arr))

def clean_up_files():
    '''
    Function that removes previous files generated from running this module
    excluding this python script or other python files if they exist
    '''
    file_list = os.listdir()
    for f in file_list:
        if not f.endswith('.py'):
            os.remove(f)

def main():
    '''
    Main routine for the python script that:
        * Cleans previousely generated text files
        * Generates new text files and outputs their contents to stdout
        * Generates two random numbers and outputs them and their product to stdout
    '''
    clean_up_files()
    make_file()
    random_num_gen()
    
if __name__ == '__main__':
    main()