#!/usr/bin/python3

import csv, sys, getopt

def main(argv):
    inputfile =''
    opts, args = getopt.getopt(argv,"hi:x:")
    for opt, arg in opts:
        if opt == '-h':
            print (sys.argv[0] + '-i <inputfile> -x exlude')
            sys.exit()
        elif opt in ("-i"):
            inputfile = arg
        elif opt in ("-p"):
            exclude_list = arg
    if inputfile == '':
        sys.exit()

    with open(inputfile, newline='', encoding='utf-8-sig') as csvdatei:
        csv_reader_object = csv.reader(csvdatei)
        for row in csv_reader_object:
            print(row[0])

if __name__ == "__main__":
    main(sys.argv[1:])

