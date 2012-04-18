import sys, os.path
import shutil

options = [["treat-wchar_t-as-typedef", "Treat wchar_t as as a typedef not as a builtin type\n"],
          ["use-sdklibs(bool)=1", "Use libraries specified by <lib,name,name> directives, but only for sdk modules.\n"]]

here = os.path.abspath(os.path.normpath("."))          
extras = os.path.join(here, "extra")
filter = ['mkb', 'mkf']
s3etag = "#s3einclude"
          
# find s3e dir
def get_s3e_dir():
    if 'S3E_DIR' in os.environ:
        s3edir = os.path.join(os.environ['S3E_DIR'])
    else:
        s3edir = os.path.abspath(os.path.normpath(os.path.join("..","s3e")))
        if not os.path.isdir(s3edir):
            return None
    return s3edir

# find wchar line
def has_option(f, option):
    f.seek(0)    
    for line in f:
        if line.find(option) == 0:
            return True            
    return False

# insert treat-wchar_t-as-typdef option
def add_option(f, option):    
    f.seek(0, 2)
    f.write(option)    
    return
    
def copy_extras(dirpath, sdkdir):
    for f in os.listdir(dirpath):
        src = os.path.join(dirpath, f)
        if os.path.isdir(src):       
            copy_extras(src, sdkdir)            
        else:
            #extra/<path> to #sdkdir/<path>
            try:
                dst = os.path.join(sdkdir, src[len(extras)+1:])
                print "Copying %s to %s" % (os.path.basename(dst), dst)            
                shutil.copy2(src, dst)
            except IOError, err:
                print str(err)
                pass
    return    
   
def add_s3einclude(mkb):
    try:
        found = False
        f = file(mkb, "r+")
        
        out = ""
        for line in f:
            if line.find(s3etag)>=0:
                found = True
                print "#s3einclude found"                
                line = 'includepath %s %s\n' % (s3einclude, s3etag)
            
            out += line
    finally:
        f.close()

    if found:
        try:
            shutil.copy2(mkb, mkb+".old")
            f = file(mkb, "w+")
            f.write(out)
        finally:
            f.close()
    return
    
def replace_mkb_paths(dirpath):
    for f in os.listdir(dirpath):
        src = os.path.join(dirpath, f)
        if os.path.isdir(src):       
            replace_mkb_paths(src)         
        else:
            farr = src.split(os.extsep)
            if len(farr) > 1:
                fext = farr[-1].lower()
                if fext in filter:
                    print "processing " + src
                    add_s3einclude(src)
    return
    
    
#########
s3edir = get_s3e_dir()
sdkdir = os.path.normpath(os.path.join(s3edir, ".."))
s3einclude = os.path.join(s3edir, "h")
s3einclude = s3einclude[s3einclude.find(":")+1:]

if s3edir == None:
    print "Could not locate s3e dir.  Try running in the sdk root folder."
else:
    print "s3e dir: " + s3edir
    
    ## OPTIONS
    docsdir = os.path.join(s3edir, "makefile_builder/docs")
    optionsfile = os.path.join(docsdir, "options.txt")
    if not os.path.exists(optionsfile):
        print "Could not locate options.txt.  Try re-installing the sdk."
    else:
        print "checking options.txt"
        f = open(optionsfile, "r+")
        try:
            counter = 0
            for option in options:
                if not has_option(f, option[0]):
                    counter = counter + 1
                    if counter == 1:
                        add_option(f, "\n")
                    print "adding option: " + option[0]
                    add_option(f, "   ".join(option))        
        finally:
            f.close()
    
    ## COPY FILES TO SDK
    copy_extras(extras, sdkdir)
    
    ## REPLACE MKB includepath 
    #replace_mkb_paths(here)
            
    print "done"

    
    