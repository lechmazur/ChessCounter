import os
import signal
import subprocess
from subprocess import Popen, PIPE
import time
fepd = open("u:/chestuci/ChestUCI.epd", "r")
epd = fepd.readlines()
fw = open("tmpout", "wb")
fr = open("tmpout", "r")

def processFen(p,fen):
    print(fen)
    loc=fen.find(';')
    if loc != -1:
       fen = fen[0:loc]
    p.stdin.write("position fen "+fen+"\n")
    p.stdin.write("isready\n")
    while True:
        data = fr.read()
        if data != '':
            print(data)
            if data.find('readyok') != -1:
                break
    p.stdin.write("go\n")
    start_time = time.time()
    elapsed_time = 0
    mate_in = -1
    best_move = ""
    found = False
    while found == False:
        data = fr.read()
        dlist = data.split('\n')
        elapsed_time = time.time() - start_time
        for line in dlist:
            if line != '':
                print(elapsed_time, line)
                if line.find('info string Mate in ')==0:
                    line = line.replace('info string Mate in ','')
                    d2 =  line.find(' found')
                    mate_in = line[0:d2]
                    
                if line.find('bestmove') != -1:
                    best_move = line.replace('bestmove ', '')
                    print('elapsed time', elapsed_time)
                    found = True
                    break

    return (elapsed_time, mate_in, best_move)

def processChest(start_at,end_at):

    p = Popen("u:/chestuci/chestUCI.exe", stdin=PIPE, stdout=fw, stderr=fw, bufsize=1, universal_newlines=True)
    p.stdin.write("uci\n")
    p.stdin.write("setoption name German value false\n")
    p.stdin.write("setoption name AllSolutions value false\n")
    p.stdin.write("setoption name HashSize value 2048\n")
    p.stdin.write("setoption name SearchDepth value 10\n")

    out = []
    out.append("problem,time,mate_in,best_move")
    for i in range(start_at,end_at):
        if epd[i].find("Defence") == -1 and epd[i] != '\n':
         (elapsed, mate_in,best_move) = processFen(p,epd[i])
         out.append(str(i)+","+str(elapsed)+","+mate_in+","+best_move)
         fout = open("u:/No Problem/chest-results.csv",'w')
         for line in out:
               fout.write(line+"\n")
         fout.close()

    p.kill()
    os.kill(p.pid, signal.CTRL_C_EVENT)
    os.kill(p.pid, signal.CTRL_BREAK_EVENT)
    subprocess.call(['taskkill', '/F', '/T', '/PID',  str(p.pid)])



#processChest(5000,5010)
processChest(4361,5000)