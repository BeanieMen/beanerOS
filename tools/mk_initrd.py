#!/usr/bin/env python3
import struct
import sys
import os

MAGIC = 0xBF

def create_initrd(output_file, input_files):
    """Create an initrd file from a list of input files."""
    
    nfiles = len(input_files)
    
    # Calculate offsets
    header_size = 4 + (nfiles * 72)  # 4 bytes for nfiles + 72 bytes per file header
    current_offset = header_size
    
    headers = []
    file_data = []
    
    for filepath in input_files:
        if not os.path.exists(filepath):
            print(f"Error: File {filepath} not found")
            sys.exit(1)
            
        filename = os.path.basename(filepath)
        if len(filename) > 63:
            filename = filename[:63]
            
        with open(filepath, 'rb') as f:
            data = f.read()
            
        headers.append({
            'magic': MAGIC,
            'name': filename,
            'offset': current_offset,
            'length': len(data)
        })
        
        file_data.append(data)
        current_offset += len(data)
        
        print(f"Adding: {filename} ({len(data)} bytes) at offset {headers[-1]['offset']}")
    
    # Write initrd
    with open(output_file, 'wb') as f:
        # Write number of files
        f.write(struct.pack('<I', nfiles))
        
        # Write headers
        for header in headers:
            f.write(struct.pack('<B', header['magic']))
            name_bytes = header['name'].encode('ascii')
            name_bytes = name_bytes.ljust(64, b'\x00')
            f.write(name_bytes)
            f.write(struct.pack('<I', header['offset']))
            f.write(struct.pack('<I', header['length']))
        
        # Write file data
        for data in file_data:
            f.write(data)
    
    print(f"\nInitrd created: {output_file}")
    print(f"Total size: {os.path.getsize(output_file)} bytes")
    print(f"Files: {nfiles}")

if __name__ == '__main__':
    if len(sys.argv) < 3:
        print("Usage: mk_initrd.py <output.img> <file1> [file2] [file3] ...")
        sys.exit(1)
    
    output_file = sys.argv[1]
    input_files = sys.argv[2:]
    
    create_initrd(output_file, input_files)
