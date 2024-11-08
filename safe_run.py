#!/usr/bin/env python

from subprocess import *
from concurrent.futures import ThreadPoolExecutor as Pool
import os
import sys
import resource
import psutil
from random import *
import time
import math
import json

from os import system
from glob import glob

call(['make','-j'])
call(['make','-j','count_tasks'])

SUCCESS, TLE, MLE, RTE, RUNNING = 0,1,2,3,-1
exit_names = ["SUCCESS", "TLE", "MLE", "RTE", "RUNNING"]

start_time = time.time()


MEMORY_LIMIT = 30*1024 * 0.95 # 30 GB
TIME_LIMIT   = 12*3600 * 0.95 # 12 hours


class Process:
    def __init__(self, cmd, timeout, maxmemory):
        fn = cmd.replace(' ','_')
        self.fout = open('store/tmp/%s.out'%fn,'w')
        self.ferr = open('store/tmp/%s.err'%fn,'w')
        print(cmd)
        sys.stdout.flush()
        self.cmd = cmd
        self.process = Popen(cmd.split(), stdout=self.fout, stderr=self.ferr, shell=False)
        self.pid = self.process.pid
        self.mp = psutil.Process(self.pid)
        self.memused, self.timeused = 0, 0
        self.start_time = time.time()
        self.timeout = timeout
        self.maxmemory = maxmemory

    def update(self):
        try:
            self.memory = self.mp.memory_info().rss/2**20
        except (psutil.ZombieProcess, psutil.NoSuchProcess):
            self.memory = 0
        self.memused = max(self.memused, self.memory)
        self.timeused = time.time()-self.start_time
        if self.memory > self.maxmemory:
            return (MLE, self.timeused, self.memused)
        if self.timeused > self.timeout:
            return (TLE, self.timeused, self.memused)
        if not self.memory:
            if self.process.wait():
                return (RTE, self.timeused, self.memused)
            else:
                return (SUCCESS, self.timeused, self.memused)
        return (RUNNING, self.timeused, self.memused)

    def __del__(self):
        self.fout.close()
        self.ferr.close()


class Command:
    def __init__(self, cmd, expected_time = TIME_LIMIT, expected_memory = MEMORY_LIMIT, slack = 1.5):
        self.cmd = cmd
        self.time = expected_time
        self.mem = expected_memory
        self.slack = slack

    def __lt__(self, other):
        return self.time < other.time


def read(fn):
    f = open(fn)
    t = f.read()
    f.close()
    return t


def runAll(cmd_list, threads):
    THREAD_LIMIT = threads

    ret_stats = {}

    # cmd_list = sorted(cmd_list)

    dt = 0.1
    running = []
    cmdi = 0

    def callback(process, status, timeused, memused):
        # assert(status != RTE)
        print(exit_names[status], process.cmd, " %.1fs"%timeused, "%.0fMB"%memused)
        sys.stdout.flush()

        ret_stats[process.cmd] = (status, timeused, memused)

    while len(running) or cmdi < len(cmd_list):
        while cmdi < len(cmd_list) and len(running) < THREAD_LIMIT:
            cmd = cmd_list[cmdi]

            taski    = int(cmd.cmd.split()[1])
            maxdepth = int(cmd.cmd.split()[2])
            mindepth = int(cmd.cmd.split()[3])
            # print("Running task: ", taski)
            cands = []
            if maxdepth != 3:
                for fn in glob("output/answer_%d_*.csv"%taski):
                    t = read(fn).strip().split('\n')
                    for cand in t[1:]:
                        img, score = cand.split()
                        cands.append((float(score), img))
                        # print("Score:" ,float(score))
                cands.sort(reverse=True)
            if len(cands) > 0:
                score, _ = cands[0]
                if score - int(score) > 0.99:
                    # print(f"Skipping task: {taski} - score: {score}" )
                    cmdi += 1
                    continue

            process = Process(cmd.cmd, cmd.time*cmd.slack, cmd.mem*cmd.slack)
            running.append(process)
            cmdi += 1

        torem = []
        mems = []
        for r in running:
            status, timeused, memused = r.update()
            mems.append(r.memory)
            if status != RUNNING:
                callback(r, status, timeused, memused)
                torem.append(r)

        if sum(mems) > MEMORY_LIMIT:
            r = running[mems.index(max(mems))]
            r.process.kill()
            callback(r, MLE, r.timeused, r.memused)
            torem.append(r)
            #THREAD_LIMIT = 1

        for r in torem:
            running.remove(r)

        time.sleep(dt)

        if time.time()-start_time >= TIME_LIMIT:
            for r in running:
                r.process.kill()
                callback(r, TLE, r.timeused, r.memused)
            break

    return ret_stats


system("mkdir -p output")
system("mkdir -p store/tmp")
system("rm -f output/answer*.csv")

if len(sys.argv) == 3:
    l = int(sys.argv[1])
    n = int(sys.argv[2])
    ntasks = n
    task_list = range(l, l+n)
else:
    ntasks = int(check_output('./count_tasks'))
    task_list = range(0, ntasks)
    print("Usage: python %s <start_task> <#tasks>"%sys.argv[0])

depth3 = []
for i in range(ntasks):
    depth3.append(Command("./run %d  3 30"%i))
    # depth3.append(Command("./run %d 3"%i, 4*60))
stats3 = runAll(depth3, 4)

# depth63 = []
# for i in range(ntasks):
#     # Watch this, as stats3 doesn't get populated correctly when commands above fail
#     status, t, m = stats3[depth3[i].cmd]
#     depth63.append(Command("./run %d 63"%i, t*2, m*2, 100))
#     # depth63.append(Command("./run %d 63"%i, 120))
# stats63 = runAll(depth63, 4)

# depth73 = []
# for i in range(ntasks):
#     status, t, m = stats3[depth3[i].cmd]
#     depth73.append(Command("./run %d 73"%i, t*2, m*2, 100))
#     # depth73.append(Command("./run %d 73"%i, 120))
# stats73 = runAll(depth73, 4)

depth4p = []
for i in range(ntasks):
    # Watch this, as stats3 doesn't get populated correctly when commands above fail
    status, t, m = stats3[depth3[i].cmd]
    depth4p.append(Command("./run %d  4 30"%i))
    depth4p.append(Command("./run %d 64 30"%i))
    depth4p.append(Command("./run %d 74 30"%i))
    depth4p.append(Command("./run %d  5 30"%i, 900, 10240, 1))
    # depth4.append(Command("./run %d 4"%i, 1200))
stats4p = runAll(depth4p, 1)

# depth64 = []
# for i in range(ntasks):
#     status, t, m = stats3[depth3[i].cmd]
#     depth64.append(Command("./run %d 64 30"%i, t*20, m*20, 2))
#     # depth64.append(Command("./run %d 63"%i, 120))
# stats64 = runAll(depth64, 4)

# depth74 = []
# for i in range(ntasks):
#     status, t, m = stats3[depth3[i].cmd]
#     depth74.append(Command("./run %d 74 30"%i, t*20, m*20, 2))
#     # depth74.append(Command("./run %d 73"%i, 120))
# stats74 = runAll(depth74, 4)

# depth4 = []
# for i in range(ntasks):
#     status, t, m = stats3[depth3[i].cmd]
#     depth4.append(Command("./run %d 4 30"%i, t*20, m*20, 2))
#     # depth4.append(Command("./run %d 4"%i, 1200))
# stats4 = runAll(depth4, 4)

combined = ["output_id,output"]
for taski in task_list:
    # print(f"Dealing with task: {taski}")
    ids = set()
    cands = []
    for fn in glob("output/answer_%d_*.csv"%taski):
        t = read(fn).strip().split('\n')
        # print(f"  Line: {t}")
        ids.add(t[0])
        for cand in t[1:]:
            img, score = cand.split()
            # print(f"  Cand: <img> {score}")
            cands.append((float(score), img))

    # assert(len(ids) == 1)
    if len(ids) == 1:
        id = ids.pop()
        cands.sort(reverse=True)
        best = []
        for cand in cands:
            score, img = cand
            if not img in best:
                best.append(img)
                if len(best) == 2:
                    break
        # Ensure that there's 2 attempts - Pierre 20241029
        while len(best) < 2: best.append('|0|')
        # print(f"  Append: {id+','+' '.join(best)}")
        combined.append(id+','+' '.join(best))
    else:
        print(f"Error: No result for task {taski}: {len(ids)}")

outf = open('submission_part.csv', 'w')
for line in combined:
    print(line, file=outf)
outf.close()

# Do the same again, but for jsons - Pierre 20241029
submission = {}
for taski in task_list:
    # print(f"Dealing with task: {taski}")
    ids = set()
    cands = []
    for fn in glob("output/answer_%d_*.json"%taski):
        with open(fn, 'r') as file:
            data = json.load(file)
        # print(data)

        for t, t_v in data.items():
            # print(f"{t}")
            items = list(t_v[0].items())
            for (_, score), (_, img) in zip(items[::2], items[1::2]):
                # print(f"{score}\n  {img}")
                ids.add(t)
                # print(f"  Task: {t} Cand: {score} <img>")
                cands.append((float(score), img))

    # assert(len(ids) == 1)
    if len(ids) == 1:
        t = ids.pop()
        task_id, output_idx = t.split('_')
        cands.sort(reverse=True)
        best = []
        # for cand in cands:
        #     score, output_idx, img = cand
        #     print(f"Sorted: {score} {output_idx} {img}")
        #     if not img in best:
        #         best.append(img)
        #         if len(best) == 2:
        #             break
        # # Ensure that there's 2 attempts - Pierre 20241029
        # while len(best) < 2: best.append([[0]])
        # print(f"  Append: {id+','+' '.join(best)}")

        # best = []
        for cand in cands:
            score, img = cand
            if img and not img in best:  # Check if pred is not an empty string
                # img_lines = img.split('|')[1:-1]  # Remove empty strings from split
                # img_matrix = [list(map(int, line)) for line in img_lines]
                # best.append(img_matrix)
                best.append(img)
                if len(best) == 2:
                    break

        attempt_1 = best[0] if len(best) > 0 else [[0]]
        attempt_2 = best[1] if len(best) > 1 else [[0]]

        if task_id not in submission:
            submission[task_id] = []

        attempt = {
            "attempt_1": attempt_1,
            "attempt_2": attempt_2
        }
        if output_idx == '0':
            submission[task_id].insert(0, attempt)
        else:
            submission[task_id].append(attempt)

        # empty_dict = {
        #     "attempt_1": [[0]],
        #     "attempt_2": [[0]]
        # }

        # for json_id in json_name:
        #     if json_id not in submission_dict:
        #         submission_dict[json_id] = []
        #         submission_dict[json_id].append(empty_dict)
    else:
        print(f"Error: No result for task {taski}: {len(ids)}")

with open('submission_part.json', 'w') as file:
    json.dump(submission, file, indent=4)
