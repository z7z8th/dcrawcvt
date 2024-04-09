#!/usr/bin/env python

import os
import sys


def reorder(infp, outfp):
    with open(infp, 'rb') as inf:
        with open(outfp, 'wb') as outf:
            while True:
                buf = bytearray(inf.read(8))
                print('read buf len ', len(buf))
                if len(buf) != 8:
                    print('read buf len ', len(buf))
                    return
                # obuf = bytearray({buf[x] for x in range(7, -1, -1) })
                buf.reverse()
                outf.write(buf)


if __name__ == '__main__':
    reorder(sys.argv[1], sys.argv[2])