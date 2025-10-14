#!/usr/bin/env python3
"""
Merge multiple compile_commands.json files into one for SonarLint
"""
import json
import glob
import os

def merge_compile_commands():
    # Find all compile_commands.json files
    compile_files = glob.glob("**/compile_commands.json", recursive=True)
    
    merged_commands = []
    
    for file_path in compile_files:
        print(f"Processing: {file_path}")
        try:
            with open(file_path, 'r') as f:
                commands = json.load(f)
                # Convert relative paths to absolute paths
                for cmd in commands:
                    if 'directory' in cmd:
                        # Make directory path absolute relative to the compile_commands.json location
                        base_dir = os.path.dirname(os.path.abspath(file_path))
                        if not os.path.isabs(cmd['directory']):
                            cmd['directory'] = os.path.join(base_dir, cmd['directory'])
                        cmd['directory'] = os.path.normpath(cmd['directory'])
                
                merged_commands.extend(commands)
        except Exception as e:
            print(f"Error processing {file_path}: {e}")
    
    # Write merged file
    output_file = "merged_compile_commands.json"
    with open(output_file, 'w') as f:
        json.dump(merged_commands, f, indent=2)
    
    print(f"Merged {len(compile_files)} files into {output_file}")
    print(f"Total compilation commands: {len(merged_commands)}")

if __name__ == "__main__":
    merge_compile_commands()