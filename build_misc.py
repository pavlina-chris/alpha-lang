# This Python file contains miscellaneous build tools for Alpha. The Makefile
# calls them as necessary. Note that I claim no copyright on this code.

def program (f):
    """
    Decorator: Make f() function like a standalone program. If it does not
    throw an error, call sys.exit() on its return value; otherwise, print
    a Unix-style error message and exit 1.
    """

    def wrap (*args, **kwargs):
        import sys
        r = 0
        try:
            r = f (*args, **kwargs)
        except Exception as e:
            exc_type, exc_obj, exc_tb = sys.exc_info ()
            sys.stderr.write ("build_misc: %s [line %d]\n"
                    % (str(e), exc_tb.tb_next.tb_lineno))
            sys.stderr.flush ()
            sys.exit (1)
        else:
            if isinstance (r, int):
                sys.exit (r)
            else:
                sys.exit (0)

    return wrap

def _get_and_remove_first_line (s):
    first, nl, rest = s.partition ('\n')
    return (first, rest)

@program
def autogen ():
    """
    Read key=value pairs from "autogen.conf"
    For each file autogen/*.auto
        Let @path = first line
        Remove first line
        For each key in autogen.conf
            Replace %%key%% by value
        Save new file as @path relative to source root
    """

    import os

    def get_keys ():
        keys = {}
        with open ("autogen.conf") as f:
            for line in f:
                # Strip comment
                line = line.partition ('#')[0].strip ()

                key, eq, value = line.partition ('=')
                key = key.strip ()
                value = value.strip ()

                if key:
                    keys[key] = value
        return keys

    keys = get_keys ()

    for file in os.listdir ("autogen"):
        if not file.endswith (".auto"):
            continue
        file = "autogen/" + file
        path = text = None
        with open (file) as f:
            path, text = _get_and_remove_first_line (f.read ())
        for key in keys:
            text = text.replace ("%%" + key + "%%", keys[key])
        with open (path, 'w') as f:
            f.write (text)

@program
def autogen_clean ():
    """
    Remove all files produced by autogen()
    """

    import os

    for file in os.listdir ("autogen"):
        if not file.endswith (".auto"):
            continue
        file = "autogen/" + file
        path = None
        with open (file) as f:
            path = _get_and_remove_first_line (f.read ())[0]
        if os.path.exists (path):
            os.unlink (path)
