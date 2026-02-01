#!/usr/bin/env python3
import serial
import sys
import time

try:
    # Open serial port
    ser = serial.Serial('COM13', 115200, timeout=2)
    time.sleep(0.5)
    
    # Flush input buffer
    ser.reset_input_buffer()
    
    # Read for 10 seconds or until we see initialization complete
    print("\n=== Serial Monitor Output ===\n")
    start = time.time()
    lines_read = 0
    
    while time.time() - start < 10:
        try:
            line = ser.readline()
            if line:
                lines_read += 1
                output = line.decode('utf-8', errors='ignore').rstrip()
                print(output)
                
                # Exit after initialization messages
                if '[AS7343]' in output and 'INITIALIZATION' in output:
                    time.sleep(0.5)
                    # Read a few more lines after initialization
                    for _ in range(20):
                        line = ser.readline()
                        if line:
                            output = line.decode('utf-8', errors='ignore').rstrip()
                            print(output)
                    break
        except:
            pass
    
    ser.close()
    print(f"\n=== Read {lines_read} lines ===")
    
except Exception as e:
    print(f"Error: {e}")
    sys.exit(1)
