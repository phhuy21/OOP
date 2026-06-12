import subprocess
import os
import time

import subprocess
import os
import time
import sys

import subprocess
import os
import time
import sys

def run_test():
    # Force python stdout to UTF-8 to support printing Vietnamese text
    sys.stdout.reconfigure(encoding='utf-8')
    print("=== STARTING AUTOMATED FUNCTIONAL VERIFICATION ===")
    
    # Ensure clean state
    if os.path.exists("data/airports.txt"):
        try:
            os.remove("data/airports.txt")
        except Exception as e:
            print(f"Warning: could not delete data/airports.txt: {e}")
            
    # Start skygate.exe
    proc = subprocess.Popen(
        ["./skygate.exe"],
        cwd="d:/LTHDT/skygate",
        stdin=subprocess.PIPE,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
        bufsize=0, # Unbuffered
        encoding="utf-8"
    )
    
    # Correct inputs sequence accounting for pause() prompts:
    inputs = [
        "n",  # Seeding demo initially? No.
        "9",  # Seed demo via menu
        "",   # Pause after seed
        "1",  # Airport menu
        "4",  # List airports
        "",   # Pause after list airports
        "0",  # Back to main menu
        "2",  # Aircraft menu
        "2",  # List aircrafts
        "",   # Pause after list aircrafts
        "0",  # Back to main menu
        "3",  # People/Crew menu
        "7",  # List people/crews
        "",   # Pause after list people
        "0",  # Back to main menu
        "5",  # Flight menu
        "11", # List flights
        "",   # Pause after list flights
        "0",  # Back to main menu
        "8",  # Save/Load menu
        "1",  # Save to file
        "",   # Pause after save
        "0",  # Back to main menu
        "0",  # Exit
        "n"   # Save on exit? No (already saved)
    ]
    
    input_str = "\n".join(inputs) + "\n"
    
    # Wait for completion or timeout
    try:
        stdout, stderr = proc.communicate(input=input_str, timeout=5)
    except subprocess.TimeoutExpired:
        print("[-] Test failed: Process timed out.")
        proc.kill()
        stdout, stderr = proc.communicate()
        
    print("--- STDOUT ---")
    print(stdout)
    print("--- STDERR ---")
    print(stderr)
    
    print("=== PROCESS COMPLETED ===")
    print(f"Exit Code: {proc.returncode}")

    
    data_files = [
        "data/airports.txt",
        "data/aircraft.txt",
        "data/pilots.txt",
        "data/groundstaff.txt",
        "data/crews.txt",
        "data/flights.txt",
        "data/passengers.txt",
        "data/gates.txt",
        "data/runways.txt",
        "data/queues.txt"
    ]

    
    all_files_exist = True
    for f in data_files:
        full_path = os.path.join("d:/LTHDT/skygate", f)
        exists = os.path.exists(full_path)
        print(f"File {f}: {'EXISTS' if exists else 'MISSING'}")
        if not exists:
            all_files_exist = False
            
    return all_files_exist and proc.returncode == 0



if __name__ == "__main__":
    success = run_test()
    if not success:
        exit(1)
