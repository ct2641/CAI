import os
import argparse
import json
from itertools import permutations
from itertools import product
from math import ceil

"""
The problem is in the paper Maddah-Ali, Mohammad Ali, and Urs Niesen. "Fundamental limits of caching." IEEE Transactions on Information Theory 60.5 (2014): 2856-2867.
"""

parser = argparse.ArgumentParser(description = 'Coded caching with N files and K users')
parser.add_argument('-N', '--N', type = int, default = 2, choices = range(1, 10), help = "number of files")
parser.add_argument('-K', '--K', type = int, default = 3, choices = range(1, 10), help = "number of users")


def get_type(N, K):
    output = [[K]]
    if N == 1:
        return output
    for k in range(K - 1, ceil(K / N) - 1, -1):
        fols = get_type(N - 1, K - k)
        for fol in fols:
            output.append([k] + fol)
    return output
    
def gen_pd_cache(N, K):
    # create file if not exists
    cai = os.path.dirname(os.path.abspath(__file__)) + '/../../'
    if not os.path.exists(cai + 'PlotTradeoff/inputfiles'):
        os.makedirs(cai + 'PlotTradeoff/inputfiles')
    fn = cai + 'PlotTradeoff/inputfiles/PD_cache{}x{}.pd'.format(N, K)
    if os.path.exists(fn):
        print("file " + fn + " already exists")
        print("Overwrite[y/n]:", end="")
        if input() == "y":
            pass
        else:
            print("Keep old PD_cache{}x{}.pd".format(N, K))
            return fn
    
    # initialize PD dict
    PD = {
        "RV": [], 
        "AL": ["M","R"], 
        "O": "M + R",
        "D": [],
        "I": [],
        "BC": [
            "H(W1) = 1",
            "H(Z1) - M <= 0" ],
        "BP": [],
        "S": []
    }

    W = ["W{}".format(n) for n in range(1, N + 1)]
    Z = ["Z{}".format(k) for k in range(1, K + 1)]
    reqs = list(product(['{}'.format(n) for n in range(1, N + 1)], repeat=K))
    X = ["X" + "".join(req) for req in reqs]
    
    # RV
    PD["RV"] = W + Z + X

    # D
    PD["D"] = [{"given":W, "dependent":Z}, {"given":W, "dependent":X}]
    for k in range(1, K + 1):
        for req in reqs:
            PD["D"].append({"given":["Z{}".format(k), "X" + "".join(req)], "dependent":["W{}".format(req[k - 1])]})

    # I
    PD["I"] = [{"given":[], "independent":W}]

    # BC
    types = get_type(N, K)
    for tp in types:
        tmp = "X" + "".join(["{}".format(n + 1) * tp[n] for n in range(len(tp))])
        """
        tmp = "X"
        n = 1
        for num in tp:
            tmp = tmp + "{}".format(n) * num
            n = n + 1
        """
        PD["BC"].append("H({}) - R <= 0".format(tmp))

    # S
    Kperms = list(permutations(list(range(K))))
    Nperms = list(permutations(list(range(N))))
    reqs = list(product([n for n in range(N)], repeat=K))
    for Nperm in Nperms:
        for Kperm in Kperms:
            Wperm = ["W{}".format(n + 1) for n in Nperm]
            Zperm = ["Z{}".format(k + 1) for k in Kperm]
            Xperm = []
            for req in reqs:
                tmp = "X"
                for k in range(K):
                    tmp = tmp + "{}".format(Nperm[req[Kperm.index(k)]] + 1)
                Xperm.append(tmp)
            PD["S"].append(Wperm + Zperm + Xperm)

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
    gen_pd_cache(N, K)