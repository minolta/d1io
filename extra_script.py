Import("env")

# build_tag = "v2.2"

# env.Replace(PROGNAME="D1io_%s" % build_tag)
# access to global construction environment
env.Replace(PROGNAME="D1io")

# print env

# Dump construction environment (for debug purpose)
# print env.Dump()