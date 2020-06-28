import os
import sys
import platform
import subprocess
from functools import partial
from multiprocessing.dummy import Pool
from shutil import copyfile, copytree

myEnv = os.environ.copy()
system = platform.system()
cwd = os.getcwd().replace('\\', '/')
parrent = cwd[0:cwd.rfind('/')]
args = sys.argv[1:]
progs = {"gcc" : "gcc" if system in ["Linux", "Darwin"] else parrent + "/cygwin64/bin/gcc.exe",
		 "g++" : "g++" if system in ["Linux", "Darwin"] else parrent + "/cygwin64/bin/g++.exe",
		 "ld" : "ld" if system in ["Linux", "Darwin"] else parrent + "/cygwin64/bin/ld.exe"}

options = ["-D SPDLOG_COMPILED_LIB", "-D LTC_SOURCE", "-D LTC_NO_ROLC"]
rebuild = False
threads = 4

if "--rebuild" in args:
	rebuild = True

if "-ipv6" in args:
	options += ["-D IPV6"]

if "-ssl" in args:
	options += ["-D __TLS__"]
	rebuild = True

if "-r" in args or "--release" in args:
	options += ["-O3"]

if "--custom-build-for-niclas" in args or "-n" in args:
	options += ["-D __CUSTOM_BUILD_FOR_NICLAS__"]

def run(cmd):
	#print(cmd)
	subprocess.run(cmd, shell=True, env=myEnv)

def prog(name):
	return progs[name]

def src():
	for root, dirs, files in os.walk("./src"):
		dir = root[2:].replace('\\', '/')
		module = dir[4:] if len(dir) > 4 else "root"
		for file in files:
			ending = file[file.find('.')+1:]
			if ending in ["c", "cpp", "h", "hpp"]:
				yield module, module + '/' + file, ending

def makedir(dir):
	if not os.path.isdir(dir):
		os.makedirs(dir)

def gcccmd(args):
	return ' '.join([prog("gcc")] + args + ["-std=c11"])
def gppcmd(args):
	return ' '.join([prog("g++")] + args + ["-std=c++17"])
def ldcmd(args):
	return ' '.join([prog("ld")] + args)
def gcc(args):
	run(gcccmd(args))
def gpp(args):
	run(gppcmd(args))
def ld(args):
	run(gcccmd(args))

def check(src, dest):
	srh1 = src[:src.rfind('.')+1] + "h"
	srh2 = src[:src.rfind('.')+1] + "hpp"
	r = False
	if os.path.isfile(src) and os.path.isfile(dest):
		if(os.path.isfile(srh1)):
			r = os.path.getmtime(srh1) > os.path.getmtime(dest)
		elif(os.path.isfile(srh2)):
			r = os.path.getmtime(srh2) > os.path.getmtime(dest)
		r = r or os.path.getmtime(src) > os.path.getmtime(dest)
	else:
		r =  True
	return r or rebuild

def compilecmd(file, compiler, args):
	src = "src/" + file
	dest = "build/" + file[:file.find('.')] + '.o'
	makedir(dest[:dest.rfind('/')])
	if file.startswith("tomcrypt"):
		args += ["-Iinclude/tomcrypt"]
	elif file.startswith("tommath"):
		args += ["-Iinclude/tommath"]
	elif file.startswith("soloud"):
		args += ["-Iinclude/soloud", "-D WITH_ALSA"]
	if check(src, dest):
		return dest, True, compiler([src, "-o", dest, "-c", "-Iinclude"] + args)
	return dest, False, ""

def linkcmd(files, target, args = []):
	for f in files:
		if not check(f, target):
			return ""
	return ldcmd(files + ["-o"] + [target] + args)

def linkdllcmd(src, dest, soname, args = []):
	if check(src, dest):
		return gcccmd([src, "-o", dest, "-shared", "-fpic", "-Wl,-soname=" + soname] + args)
	return ""

def buildProgcmd(src, dest, args = []):
	if check(src, dest):
		return gppcmd([src, "-o", dest, "-Iinclude"] + args)
	return ""

def compile(file, compiler):
	res, built, cmd = compilecmd(file, compiler)
	run(cmd)
	return res, built

def link(files, target, args = []):
	run(linkcmd(files, target, args))

def linkdll(files, target, soname, args = []):
	run(linkdllcmd(files, target, soname, args))

def buildProg(src, dest, args = []):
	run(buildProgcmd(src, dest, args))

def runCommandList(commands):
	pool = Pool(threads)
	pool.map(partial(run), commands)
	pool.close()
	pool.join()


def buildYeetlib():
	commands = []
	libs = []
	relink = rebuild
	for m,f,e in src():
		if e in ["c", "cpp"]:
			res, built, cmd = compilecmd(f, {"c":gcccmd, "cpp":gppcmd}[e], ["-fpic", "-Wall"] + options)
			commands.append(cmd)
			libs.append(res)
			relink = relink or built

	runCommandList([cmd for cmd in commands if cmd != ""])

	if relink or not os.path.isfile("build/yeet.o") or not os.path.isfile("build/yeet.so"):
		link(libs, "build/yeet.o", ["-relocatable"])
		linkdll("build/yeet.o", "build/yeet.so", "./yeet.so", [])

def buildServer():
	if not os.path.isfile("build/server.o") or os.path.getmtime("server.cpp") > os.path.getmtime("build/server.o") or os.path.getmtime("server.hpp") > os.path.getmtime("build/server.o"):
		gpp(["server.cpp", "-c", "-o", "build/server.o", "-Iinclude", "-fpic"] + options)
	gpp(["build/server.o", "-o", "build/server", "build/yeet.so", "-lpthread", "-ldl"] + options)
	linkdll("build/server.o", "build/server.so", "./server.so", [])

def buildPlugin():
	if not os.path.isfile("build/plugin.o") or os.path.getmtime("plugin.cpp") > os.path.getmtime("build/plugin.o") or os.path.getmtime("plugin.hpp") > os.path.getmtime("build/plugin.o"):
		gpp(["plugin.cpp", "-c", "-o", "build/plugin.o", "-Iinclude", "-fpic"] + options)
	linkdll("build/plugin.o", "build/plugin.so", "./plugin.so", ["build/yeet.so", "build/server.so"])

def copyResources():
	copyfile("plugins.json", "build/plugins.json")
	copyfile("init.sql", "build/init.sql")

	if(os.path.isdir("www") and not os.path.isdir("build/www")):
		copytree("www", "build/www")
		copyfile("README.md", "build/www/README.md")

if __name__ == "__main__":
	buildYeetlib()
	buildServer()
	buildPlugin()
	copyResources()