import resource, datetime, subprocess, os, sys, select

from utils.terminal import purple

def set_memory_limit(memlim_gb: int):
	"""
	Function that sets limit on memory when executable starts.
	"""
	soft, hard = resource.getrlimit(resource.RLIMIT_AS)

	print(f"Setting memory limit to {memlim_gb}GB")
	memlim_gb_in_bytes = memlim_gb * 1024 * 1024 * 1024

	resource.setrlimit(resource.RLIMIT_AS, (memlim_gb_in_bytes, hard))

def run_program(
		bin: list[str], # the command line as a list of strings
        input_string: str, # the input string to be passed to the program
        memlim_gb: int, # memory limit in GB
		silent=False # whether to print the output to the console
    ):
	"""
	Run a program with the given command line and input string.
	"""
	t_start = datetime.datetime.now()
	
    # Print the command line.
	# if not silent: print(purple("command:" + " ".join(bin)))
	# if not silent: print(purple("input:" + input_string))
	
    # Start the process.
	process = subprocess.Popen(
		bin, 
		stderr=subprocess.PIPE, 
		stdout=subprocess.PIPE,
		stdin=subprocess.PIPE, 
		preexec_fn=set_memory_limit(memlim_gb),
		universal_newlines = True
	)
	
    # Write input to STDIN.
	if input_string != "": process.stdin.write(input_string) # Write to STDIN.
	process.stdin.flush()

	stderr_string = ""
	stdout_string = ""
	readable = { process.stdout.fileno(): 1, process.stderr.fileno(): 2 }
	while readable:
		for fd in select.select(readable, [], [])[0]:
			data = os.read(fd, 1024) # read available
			if not data: # EOF
				del readable[fd]
			else:
				if readable[fd] == 1: stdout_string += data.decode("utf-8")
				else:
					stderr_string += data.decode("utf-8")
					if not silent: sys.stdout.buffer.write(data)
					if not silent: sys.stdout.buffer.flush()				
	exit_code = process.wait()
	
	t_end = datetime.datetime.now()
	process.stdout.close()
	process.stderr.close()

	return {
		"exit_code": exit_code, 
		"stdout": stdout_string, 
		"stderr": stderr_string, 
		"time": (t_end-t_start).total_seconds() 
    }
