from SCons.Script import Import
Import("env")

import os, sys
root = env["PROJECT_DIR"]

# add these paths so 'wheezy' and 'minify' can be imported directly
sys.path.insert(0, os.path.join(root, "python", "external"))
# if you split folders, you can also add explicit subdirs:
# sys.path.insert(0, os.path.join(root, "python", "external", "wheezy"))
# sys.path.insert(0, os.path.join(root, "python", "external", "minify"))
