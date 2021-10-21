#!/usr/bin/env python3

import sys
import os
import random

def generateFiles(datas):
    filelist = {}

    for data in datas:
        filelist[data] = 0
        count = random.randrange(1, 3)
        for i in range(count):
            filelist[data] += 1 # increment count for this data
            f = open(data + str(i), "w")
            f.write(data)
            f.close
    return filelist

def main():
    if len(sys.argv) != 2:
        print("Usage: ./genTestDir.py <path>")
        exit()

    datas = ["a", "b", "c", "d", "e", "f"]

    # take path from args
    path = sys.argv[1]
    os.chdir(path)

    filelist = generateFiles(datas)
    print(filelist)
    print("Expected")
    print(sum(filelist.values()))
    print(sum(len(data) * count for data, count in filelist.items()))
    print(len(filelist))
    print(sum(len(data) for data in filelist.keys()))
    print()

    exit()
    
if __name__ == '__main__':
    main()
