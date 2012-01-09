# qsub.py
# Author: Jordan Boyd-Graber
#
# This script allows you to submit jobs to the cluster and offers the
# following advantages to using the command line qsub directly:
# 1. You can programmatically add dependencies (i.e. make a job run after
#       the other)
# 2. It forces you to be nice and doesn't let you hog the entire cluster
#       (It forces you to leave unallocated nodes)


import os
import sys
from string import strip

CLUSTER_LOCATION = ".umiacs.umd.edu"
DEFAULT_MAX_QUEUE_JOBS = 16
CLUSTER_SIZE = 48

USER = os.popen('whoami').read().strip()

def clean_name(s):
    """
    Torque doesn't allow process names to start with non alpha characters;
    this creates clean versions of names.
    """
    first_good = 0
    for i in s:
        if i.isalpha():
            break
        first_good += 1
    return s[first_good:]

# This has been disabled; fix me
def blocking_job(max_jobs):
    # In order to filter your jobs, we need to know who you are
    block_user = os.popen("whoami").read().strip()

    # Get all the jobs in the queue
    jobs = os.popen("qstat").read().split("\n")

    jobs.reverse()
    job_name = ""
    nJobs = 0
    for i in jobs:
        fields = i.split()
        if len(fields) < 4:
            continue
        user = fields[2]
        status = fields[4]
        if user == block_user:
            nJobs += 1
            job_name = fields[0] + CLUSTER_LOCATION
            if nJobs >= max_jobs:
                print "Block on ", job_name, " max jobs=", max_jobs
                return [job_name]
        # find last job that's yours

    return []


# Takes a script name and a list of dependencies returns a job ID
def qsub(script_name, max_jobs, dependency_list=[], submit=True):

    # There might be too many jobs on the cluster; we don't want to
    # monopolize, so we'll back off and wait for them to finish

    congestion_list = blocking_job(max_jobs)

    print dependency_list, congestion_list

    # If this job already depends on another job, we'll depend on it
    # for throttling, not the blocking job
    if len(dependency_list) == 0:
        dependency_list = dependency_list

    s = "qsub"
    if len(dependency_list) > 0:
        s += " -W depend=afterok"
        for i in dependency_list:
            s+= ":" + i
    if len(congestion_list) > 0:
        s += " -W depend=afterany"
        for i in congestion_list:
            s+= ":" + i

    s += " " + script_name
    if CLUSTER_LOCATION == ".umiacs.umd.edu":
        s += " -q batch"

    print "COMMAND:", s,
    val = ""
    if submit:
        val = strip(os.popen(s).read())
    print "VAL: ", val
    return val

def mkdir(dirname):
    try:
        os.mkdir(dirname)
    except OSError:
        print "Directory", dirname, "exists"

base_template = """#!/bin/bash
#PBS -N %s_~~~jobname~~~
#PBS -l mem=~~~mem~~~,walltime=~~~wall~~~
#PBS -M %s@umiacs.umd.edu
#PBS -m ae
#PBS -k oe
#PBS -V
""" % (USER, USER)

def write_file(d, template, delete, submit, max_jobs=25):
    filename = "/tmp/%s/qsub-scripts/" % USER + d["jobname"]

    mkdir("/tmp/%s/qsub-scripts/" % USER)

    for ii in d:
        template = template.replace("~~~%s~~~" % ii, str(d[ii]))

    o = open(filename, 'w')
    o.write(template)
    o.close()

    if submit:
        print "Submitting ...", filename,
        qsub(filename, max_jobs)

    if delete:
        print "deleted"
        os.remove(filename)
    print ""

if __name__ == "__main__":
    from topicmod.util import flags

    mkdir("/tmp/%s" % USER)
    mkdir("/tmp/%s/qsub-scripts" % USER)

    flags.define_string("template", "", "Where we read the template file from")
    flags.define_dict("args", {}, "Substitute values for the template")
    flags.define_dict("defaults", {}, "Default args")
    flags.define_string("wall", "24:00:00", "The wall time")
    flags.define_string("name", "", "Name given to job on cluster")
    flags.define_string("mem", "4gb", "How much memory we give")
    flags.define_int("max_jobs", 16, "Number of simultaneous jobs on cluster")
    flags.define_bool("delete_scripts", True, "Do we delete after we're done?")
    flags.define_bool("submit", True, "Do we submit")

    flags.InitFlags()
    template = open(flags.template).read()
    d = flags.defaults

    d["wall"] = flags.wall
    d["mem"] = flags.mem
    for ii in flags.args:
        d[ii] = flags.args[ii]

    if flags.name:
        d["name"] = flags.name

    if not "name" in d:
        d["name"] = "_".join(d[x] for x in flags.args)
        if d["name"] == "":
            d["name"] = USER

    d["jobname"] = d["name"].replace("/", "-")

    write_file(d, base_template + template, flags.delete_scripts, flags.submit)
