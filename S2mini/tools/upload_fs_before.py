Import("env")
import subprocess
import sys

def before_upload(source, target, env):
    print("\n" + "="*60)
    print("Uploading filesystem first...")
    print("="*60 + "\n")
    
    project_dir = env.get("PROJECT_DIR")
    
    # Build filesystem
    print("Building filesystem...")
    buildfs_cmd = [sys.executable, "-m", "platformio", "run", "--target", "buildfs", 
                   "--environment", "lolin_s2_mini", "--project-dir", project_dir]
    result = subprocess.call(buildfs_cmd)
    if result != 0:
        print("\nERROR: Filesystem build failed!")
        env.Exit(1)
    
    # Upload filesystem
    print("\nUploading filesystem to device...")
    uploadfs_cmd = [sys.executable, "-m", "platformio", "run", "--target", "uploadfs",
                    "--environment", "lolin_s2_mini", "--project-dir", project_dir]
    result = subprocess.call(uploadfs_cmd)
    if result != 0:
        print("\nERROR: Filesystem upload failed!")
        env.Exit(1)
    
    print("\n" + "="*60)
    print("Filesystem uploaded successfully!")
    print("Now uploading firmware...")
    print("="*60 + "\n")

env.AddPreAction("upload", before_upload)



