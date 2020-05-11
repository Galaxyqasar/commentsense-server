import os
import sys
import platform
import subprocess
from shutil import copyfile, copytree

myEnv = os.environ.copy()
system = platform.system()
cwd = os.getcwd().replace('\\', '/')
parrent = cwd[0:cwd.rfind('/')]
args = sys.argv[1:]
progs = {"gcc" : "gcc" if system in ["Linux", "Darwin"] else parrent + "/cygwin64/bin/gcc.exe",
		 "g++" : "g++" if system in ["Linux", "Darwin"] else parrent + "/cygwin64/bin/g++.exe",
		 "ld" : "ld" if system in ["Linux", "Darwin"] else parrent + "/cygwin64/bin/ld.exe"}

options = ["-D SPDLOG_COMPILED_LIB", "-O0", "-D LTC_SOURCE", "-D LTC_NO_ROLC", "-D ASIO_SEPARATE_COMPILATION", "-D ASIO_STANDALONE"]
link = []
rebuild = False

if "--rebuild" in args:
	rebuild = True

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

def gcc(args):
	subprocess.run(' '.join([prog("gcc")] + args + ["-std=c11"]), shell=True, env=myEnv)
def gpp(args):
	subprocess.run(' '.join([prog("g++")] + args + ["-std=c++17"]), shell=True, env=myEnv)
def ld(args):
	subprocess.run(' '.join([prog("ld")] + args), shell=True, env=myEnv)

def check(src, dest):
	srh1 = src[:src.rfind('.')+1] + "h"
	srh2 = src[:src.rfind('.')+1] + "hpp"
	r = False
	if os.path.isfile(src) and os.path.isfile(dest):
		if(os.path.isfile(srh1)):
			r =  os.path.getmtime(srh1) > os.path.getmtime(dest)
		elif(os.path.isfile(srh2)):
			r =  os.path.getmtime(srh2) > os.path.getmtime(dest)
		else:
			r =  os.path.getmtime(src) > os.path.getmtime(dest)
	elif not os.path.isfile(dest):
		r =  True
	return r or rebuild

def compile(file, compiler, args):
	src = "src/" + file
	dest = "build/" + file[:file.find('.')] + '.o'
	makedir(dest[:dest.rfind('/')])
	if file.startswith("tomcrypt"):
		args += ["-Iinclude/tomcrypt"]
	if file.startswith("tommath"):
		args += ["-Iinclude/tommath"]
	if check(src, dest):
		print("compiling ", src, "->", dest)
		compiler([src, "-o", dest, "-c", "-Iinclude"] + args)
	return dest

def link(files, target, args):
	for f in files:
		if check(f, target):
			print("linking ", target)
			ld(files + ["-o"] + [target] + args)
			return

def linkdll(src, dest, soname, args):
	if check(src, dest):
		print("linking ", dest)
		gcc([src, "-o", dest, "-shared", "-fpic", "-Wl,-soname=" + soname] + args)

def buildProg(src, dest, args):
	if check(src, dest):
		print("building ", dest)
		gpp([src, "-o", dest, "-Iinclude"] + args)

libs = []
for m,f,e in src():
	if e in ["c", "cpp"]:
		res = compile(f, {"c":gcc, "cpp":gpp}[e], ["-fpic", "-Wall"] + options)
		libs.append(res)


link(libs, "build/yeet.o", ["-relocatable"])
linkdll("build/yeet.o", "build/yeet.so", "./yeet.so", [])

if not os.path.isfile("build/server.o") or os.path.getmtime("server.cpp") > os.path.getmtime("build/server.o") or os.path.getmtime("server.hpp") > os.path.getmtime("build/server.o"):
	gpp(["server.cpp", "-c", "-o", "build/server.o", "-Iinclude", "-fpic"] + options)
gpp(["build/server.o", "-o", "build/server", "build/yeet.so", "-lpthread", "-ldl"] + options)
linkdll("build/server.o", "build/server.so", "./server.so", [])

if not os.path.isfile("build/plugin.o") or os.path.getmtime("plugin.cpp") > os.path.getmtime("build/plugin.o") or os.path.getmtime("plugin.hpp") > os.path.getmtime("build/plugin.o"):
	gpp(["plugin.cpp", "-c", "-o", "build/plugin.o", "-Iinclude", "-fpic"] + options)
linkdll("build/plugin.o", "build/plugin.so", "./plugin.so", ["build/yeet.so", "build/server.so"])

copyfile("plugins.json", "build/plugins.json")
copyfile("init.sql", "build/init.sql")

if(os.path.isdir("www") and not os.path.isdir("build/www")):
	copytree("www", "build/www")
	copyfile("README.md", "build/www/README.md")
