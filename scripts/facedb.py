#!/usr/bin/env python3

from pathlib import Path
import struct
import argparse

version = 4

parser = argparse.ArgumentParser(description="Face database builder")
parser.add_argument('name', type=str, help='name of the person for the database')
parser.add_argument('num_face_features', type=int, default=512, help='number of tensor face features (usually 2nd dimensions of 1st tensor)')
parser.add_argument('num_lvns_features', type=int, default=32, help='number of tensor liveliness features (usually 2nd dimensions of 2nd tensor)')
parser.add_argument('tensorbins', nargs='+', type=str, help='a list of tensor binaries from which to create the database')

args = parser.parse_args()

name = args.name
n_features = args.num_face_features
n_lvns_features = args.num_lvns_features
tensorbins = args.tensorbins

with open("face.bin", mode='wb') as file:
    data = struct.pack("I", version) + struct.pack("I", n_features) + struct.pack("I", n_lvns_features)
    file.write(data)

    file.write(name.encode('ascii'))
    paddingsize = 20 - len(name)

    while paddingsize > 0:
        file.write(b'\x00')
        paddingsize -= 1

    n_feature_bytes = n_features * 4
    n_lvns_bytes = n_lvns_features * 4

    data = Path(tensorbins[0]).read_bytes()
    file.write(data[n_feature_bytes:(n_feature_bytes + n_lvns_bytes)])

    n_templates = len(tensorbins)
    file.write(struct.pack("I", n_templates))

    for tensorbin in tensorbins:
        data = Path(tensorbin).read_bytes()

        file.write(data[:n_feature_bytes])
        file.write(data[:n_feature_bytes])