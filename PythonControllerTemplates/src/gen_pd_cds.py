import os
import argparse
import json
from itertools import permutations
from itertools import product
from math import ceil

"""
The problem is explained in the paper "Conditional Disclosure of Secrets: A Noise and Signal Alignment Approach", Zhou Li, Hua Sun
https://arxiv.org/abs/2002.05691
"""

parser = argparse.ArgumentParser(description = 'Conditional disclosure of secrets with X and Y symbols')
parser.add_argument('-G', '--Graph', type = str, default="")
parser.add_argument('-K', '--K', type = int, default=4, help="the number of states")

def gen_pd_cds(Graph, K):
    # X, Y = len(Graph), len(Graph[0])
    X, Y = K, K

    # create file if not exists
    if not os.path.exists('PlotTradeoff/inputfiles'):
        os.makedirs('PlotTradeoff/inputfiles')
    fn = 'PlotTradeoff/inputfiles/PD_cds{}x{}.pd'.format(X, Y)
    if os.path.exists(fn):
        print("file " + fn + " already exists")
        # return fn
    
    # initialize PD dict
    PD = {
        "RV": ["S", "Z"], 
        "AL": ["N"], 
        "O": "N",
        "D": [],
        "I": [],
        "BC": [],
        "BP": [],
        "S": []
    }

    A = ["A{}".format(x) for x in range(1, X + 1)]
    B = ["B{}".format(y) for y in range(1, Y + 1)]
    
    # RV
    PD["RV"] = ["S", "Z"] + A + B

    # D
    for x in range(1, X + 1):
        PD["D"].append({"given":["S","Z"], "dependent":["A{}".format(x)]})
    for y in range(1, Y + 1):
        PD["D"].append({"given":["S","Z"], "dependent":["B{}".format(y)]})
    
    for y in range(1, Y):
        PD["D"].append({"given":["A{}".format(y), "B{}".format(y)], "dependent":["S"]})
        PD["D"].append({"given":["A{}".format(y + 1), "B{}".format(y)], "dependent":["S"]})
    PD["D"].append({"given":["A{}".format(Y), "B{}".format(Y)], "dependent":["S"]})
    

    # BC
    PD["BC"].append("H(S) = 1")
    """
    PD["BC"].append("H(S|A{},B{}) - H(S) = 0".format(1, Y))
    PD["BC"].append("H(S|A{},B{}) - H(S) = 0".format(X - 1, Y))
    PD["BC"].append("H(S|A{},B{}) - H(S) = 0".format(1, Y - 2))

    PD["BC"].append("H(S|A{},B{}) - H(S) = 0".format(2, Y - 1))
    PD["BC"].append("H(S|A{},B{}) - H(S) = 0".format(X, Y - 3))
    for y in range(1, Y - 3):
        PD["BC"].append("H(S|A{},B{}) - H(S) = 0".format(y + 2, y))
    """
    """
    PD["BC"].append("H(S|A{},B{}) - H(S) = 0".format(1, K))
    PD["BC"].append("H(S|A{},B{}) - H(S) = 0".format(1, K//2))
    PD["BC"].append("H(S|A{},B{}) - H(S) = 0".format(K//2 + 1, K))
    for k in range(1, K + 1):
        if k == K//2:
            continue
        PD["BC"].append("H(S|A{},B{}) - H(S) = 0".format(K + 1 - k, k))
    """
    #"""
    for x in range(1, X + 1):
        for y in range(1, Y + 1):
            if x == y or x == y + 1:
                PD["D"].append({"given":["A{}".format(x), "B{}".format(y)], "dependent":["S"]})
                # PD["BC"].append("H(S|A{},B{}) = 0".format(x, y))
            else:
                PD["BC"].append("H(S|A{},B{}) - H(S) = 0".format(x, y))
    #"""

    for x in range(1, X + 1):
        PD["BC"].append("H(A{}) - N <= 0".format(x))
    for y in range(1, Y + 1):
        PD["BC"].append("H(B{}) - N <= 0".format(y))
    """
    PD["BC"].append("H(A1) - N <= 0")
    PD["BC"].append("H(A1) - H(B1) = 0")
    for x in range(1, X):
        PD["BC"].append("H(A{}) - H(A{}) = 0".format(x, X))
    for y in range(1, Y):
        PD["BC"].append("H(B{}) - H(B{}) = 0".format(y, Y))
    """


    # S
    """
    tmp = ["S", "Z"] + ["A{}".format(k) for k in range(1, K + 1)] + ["B{}".format(k) for k in range(1, K + 1)]
    PD["S"].append(tmp)
    tmp = ["S", "Z"] + ["B{}".format(k) for k in range(K, 0, -1)] + ["A{}".format(k) for k in range(K, 0, -1)]
    PD["S"].append(tmp)
    """

    # I
    PD["I"] = [{"given":[], "independent":["S", "Z"]}]

    # BP
    # PD["BP"] = ["{}N >= {}".format(2*K - 2, 2 * K - 1)]
    PD["BP"] = ["5N >= 6"]

    # save PD in json format
    with open(fn, 'w+') as fp:
        fp.write("PD\n")
        json.dump(PD, fp, indent=4)
    print("create PD file " + fn)
    return fn

if __name__ == "__main__":
    # read args
    args = parser.parse_args()
    Graph = args.Graph
    K = args.K
    K = 3

    # generate pd file
    # fn = gen_pd_cds(Graph, K)
    
    import subprocess
    for K in range(3, 4):
        fn = gen_pd_cds(Graph, K)
        name = 'cds{}'.format(K)
        subprocess.run(["CplexCompute/cplexcompute.out", fn, "prove"])
        """
        with open('PlotTradeoff/output/Prove_sym_' + name + '.txt', 'w') as fout:
            subprocess.run(["CplexCompute/cplexcompute.out", fn, "prove"], stdout=fout, text=True)
        """