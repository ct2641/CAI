import os
import argparse
import json
from itertools import permutations
from itertools import combinations

"""
The problem can be found in the paper by Tian, Chao. "On the storage cost of private information retrieval." IEEE Transactions on Information Theory (2020).
"""

parser = argparse.ArgumentParser(description = 'PIR with N servers and K messages')
parser.add_argument('-N', '--N', type = int, default = 3, choices = range(2, 10), help = "number of servers")
parser.add_argument('-K', '--K', type = int, default = 2, choices = range(2, 10), help = "number of messages")

def gen_pd_pir(N, K):
    # create file if not exists
    cai = os.path.dirname(os.path.abspath(__file__)) + '/../../'
    if not os.path.exists(cai + 'PlotTradeoff/inputfiles'):
        os.makedirs(cai + 'PlotTradeoff/inputfiles')
    fn = cai + 'PlotTradeoff/inputfiles/PD_PIR{}x{}.pd'.format(N, K)
    if os.path.exists(fn):
        print("file " + fn + " already exists")
        print("Overwrite[y/n]:", end="")
        if input() == "y":
            # print("Overwrite PD_PIR{}x{}.pd".format(N, K))
            pass
        else:
            print("Keep old PD_PIR{}x{}.pd".format(N, K))
            return fn

    # initialize PD
    PD = {
        "RV": ["F"], 
        "AL": ["A","B"], 
        "O": "A + B",
        "D": [],
        "I": [],
        "BC": [],
        "BP": [],
        "S": []
    }

    W = ["W{}".format(k) for k in range(1, K + 1)]
    S = ["S{}".format(n) for n in range(1, N + 1)]
    Q = ["Q{}{}".format(n, k) for n in range(1, N + 1) for k in range(1, K + 1)]
    A = ["A{}{}".format(n, k) for n in range(1, N + 1) for k in range(1, K + 1)]

    # PD
    PD["RV"] = ["F"] + W + S + Q + A

    # D
    PD["D"] = [{"dependent":S, "given":W}]
    PD["D"].append({"dependent":Q, "given":["F"]})
    for n in range(1, N + 1):
        for k in range(1, K + 1):
            PD["D"].append({"dependent":["A{}{}".format(n, k)], 
                "given":["S{}".format(n), "Q{}{}".format(n, k)]})
    for k in range(1, K + 1):
        PD["D"].append({"dependent":["W{}".format(k)], 
            "given":["F"] + ["A{}{}".format(n, k) for n in range(1, N + 1)]})

    # I
    PD["I"] = [{"given":[] , "independent":["F"] + W}]

    # BC
    PD["BC"] = ["H(S{}) - A <= 0".format(n) for n in range(1, N + 1)]
    PD["BC"].append("H(A12|F) - B <= 0")
    PD["BC"].append("H(W1) = 1")
    
    for n in range(1, N + 1):
        for k in range(2, K + 1):
            PD["BC"].append("H(Q{}1) - H(Q{}{}) = 0".format(n, n, k))
    
    Ssets = ["S{}".format(n) for n in range(1, N + 1)]
    Wsets = ["W{}".format(k) for k in range(1, K + 1)]
    for cntS in range(N + 1):
        for cntW in range(K):
            if cntS == 0 and cntW == 0:
                continue
            for Ss in list(combinations(Ssets, cntS)):
                for Ws in list(combinations(Wsets, cntW)):
                    if cntS == 0:
                        cond = ",".join(Ws)
                    elif cntW == 0:
                        cond = ",".join(Ss)
                    else:
                        cond = ",".join(Ss) + "," + ",".join(Ws)
                    for n in range(1, N + 1):
                        for k in range(2, K + 1):
                            PD["BC"].append("H(A{}1|{}) - H(A{}{}|{}) = 0".format(n, cond, n, k, cond))
                    for n in range(1, N + 1):
                        for k in range(2, K + 1):
                            PD["BC"].append("H(A{}1,Q{}1|{}) - H(A{}{},Q{}{}|{}) = 0".format(n, n, cond, n, k, n, k, cond))
                            PD["BC"].append("H(A{}1|F,{}) - H(A{}{}|F,{}) = 0".format(n, cond, n, k, cond))

    # S
    Nperms = list(permutations(list(range(1, N + 1))))
    Kperms = list(permutations(list(range(1, K + 1))))
    for Nperm in Nperms:
        for Kperm in Kperms:
            Wperm = ["W{}".format(k) for k in Kperm]
            Sperm = ["S{}".format(n) for n in Nperm]
            Qperm = ["Q{}{}".format(n, k) for n in Nperm for k in Kperm]
            Aperm = ["A{}{}".format(n, k) for n in Nperm for k in Kperm]
            PD["S"].append(["F"] + Wperm + Sperm + Qperm + Aperm)

    
    # save PD in json format
    with open(fn, 'w+') as fp:
        fp.write("PD\n")
        json.dump(PD, fp, indent=4)
    print("create PD file " + fn)
    return fn

if __name__ == "__main__":
    # read args
    args = parser.parse_args()
    N, K = args.N, args.K

    # generate pd file
    gen_pd_pir(N, K)