# Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
# SPDX-License-Identifier: BSD-3-Clause-Clear

import cv2
import sys
import os
import numpy as np

def letterbox(im, new_shape=(640, 640), color=(114, 114, 114), auto=True, scale_fill=False, scale_up=True, stride=32):
    """
    Resize and pad image while meeting stride-multiple constraints.
    """
    shape = im.shape[:2]  # current shape [height, width]
    if isinstance(new_shape, int):
        new_shape = (new_shape, new_shape)
    
    # Scale ratio (new / old)
    r = min(new_shape[0] / shape[0], new_shape[1] / shape[1])
    if not scale_up:  # only scale down, do not scale up (for better val mAP)
        r = min(r, 1.0)
    
    # Compute padding
    ratio = r, r  # width, height ratios
    new_unpad = int(round(shape[1] * r)), int(round(shape[0] * r))
    dw, dh = new_shape[1] - new_unpad[0], new_shape[0] - new_unpad[1]  # wh padding
    
    if auto:  # minimum rectangle
        dw, dh = np.mod(dw, stride), np.mod(dh, stride)  # wh padding
    elif scale_fill:  # stretch
        dw, dh = 0.0, 0.0
        new_unpad = (new_shape[1], new_shape[0])
        ratio = new_shape[1] / shape[1], new_shape[0] / shape[0]  # width, height ratios
    
    dw /= 2  # divide padding into 2 sides
    dh /= 2
    
    if shape[::-1] != new_unpad:  # resize
        im = cv2.resize(im, new_unpad, interpolation=cv2.INTER_LINEAR)

    top, bottom = int(round(dh - 0.1)), int(round(dh + 0.1))
    left, right = int(round(dw - 0.1)), int(round(dw + 0.1))
    im = cv2.copyMakeBorder(im, top, bottom, left, right, cv2.BORDER_CONSTANT, value=color)  # add border
    return im

def show_help():
    """
    Display help message.
    """
    help_message = """
    Usage: python preprocess.py <input_dir> <output_dir> <is_nchw> <is_uint8>
    
    Arguments:
    <input_dir>  : Path to the input directory containing image files.
    <output_dir> : Path to the output directory to save processed files.
    <is_nchw>    : Flag to indicate if the output should be in NCHW format (1 for True, 0 for False).
    <is_uint8>   : Flag to indicate if the output should be in uint8 format (1 for True, 0 for False).
    
    Example:
    python preprocess.py input_directory output_directory 1 0
    """
    print(help_message)

def main():
    if len(sys.argv) != 5:
        show_help()
        sys.exit(1)
    
    input_dir = sys.argv[1]
    output_dir = sys.argv[2]
    is_nchw = int(sys.argv[3])
    is_uint8 = int(sys.argv[4])

    if not os.path.exists(output_dir):
        os.makedirs(output_dir)

    for filename in os.listdir(input_dir):
        if filename.lower().endswith(('.png', '.jpg', '.jpeg', '.bmp', '.tiff')):
            input_path = os.path.join(input_dir, filename)
            output_path = os.path.join(output_dir, os.path.splitext(filename)[0] + '.raw')

            image = cv2.imread(input_path)
            
            # Keep aspect ratio and resize
            img = letterbox(image, 640, stride=32, auto=False)

            # BGR -> RGB
            img = img[:, :, ::-1]

            if is_nchw:
                img = np.transpose(img, (2, 0, 1))
            print(f"Processed {filename}: {img.shape}")

            if is_uint8:
                numpy_img = np.asarray(img).astype(np.uint8)
            else:
                img = img / 255
                numpy_img = np.asarray(img).astype(np.float32)
            
            numpy_img.tofile(output_path)

if __name__ == "__main__":
    main()
