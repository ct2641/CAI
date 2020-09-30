"""
py-ctrl script
1. generate problem PD file
    1.1 save PD file in /inputfiles
2. solve convex hull
    2.1 save hull information in /output
    2.2 show figure for 10 sec
    2.3 save figure in /output
"""

import os
import subprocess
import argparse
import matplotlib.pyplot as plt


parser = argparse.ArgumentParser(description = 'Plot tradeoff')

# parser.add_argument('-S', '--Solver', type = int, choices = [0, 1], default = 0, help = "0: Cplex\n1: Gurobi")

parser.add_argument('-P', '--Problem', type = int, choices = [1, 2, 3], default = 1, help=" 1: Coded Caching\n 2: Private Information Retrieval\n 3: Symmetric Private Information Retrieval")

parser.add_argument('-N1', '--N1', type = int, choices = range(2, 10), default = 2, help = "number of files in coded caching")
parser.add_argument('-K1', '--K1', type = int, choices = range(2, 10), default = 3, help = "number of users in coded caching")

parser.add_argument('-N2', '--N2', type = int, choices = range(1, 10), default = 2, help = "number of servers in private information retrieval")
parser.add_argument('-K2', '--K2', type = int, choices = range(1, 10), default = 2, help = "number of files in private information retrieval")

parser.add_argument('-N3', '--N3', type = int, choices = range(1, 10), default = 2, help = "number of servers in symmetric private information retrieval")
parser.add_argument('-K3', '--K3', type = int, choices = range(1, 10), default = 2, help = "number of files in symmetric private information retrieval")


parser.add_argument('-IP', '--InPt', type = str, help = "list of achievable points, e.g. \"(1,1);(1.25,0.85)\"", default=None)


if __name__ == "__main__":
    # directory of CAI repository
    cai = os.path.dirname(os.path.abspath(__file__)) + "/../../"

    #### HERE
    # You might need to change these lines:
    # 1. directory of solver
    SOLVER = cai + "CplexCompute/cplexcompute.out" 
    # 2. duration of the convex hull figure pausing in sec
    PAUSE = 10

     # read args
    args = parser.parse_args()

     # generate PD file
    print("Genearte PD file")
    if args.Problem == 1:
        from gen_pd_cache import gen_pd_cache
        fn = gen_pd_cache(args.N1, args.K1)
        title = "Coded Caching with {} files and {} users".format(args.N1, args.K1)
        xlabel = "Storage"
        ylabel = "Download"
        name = "cache{}x{}".format(args.N1, args.K1)
        
    elif args.Problem == 2:
        from gen_pd_pir import gen_pd_pir
        fn = gen_pd_pir(args.N2, args.K2)
        xlabel = "Storage"
        ylabel = "Download"
        title = "Private Information Retrieval with {} servers and {} files".format(args.N2, args.K2)
        name = "PIR{}x{}".format(args.N2, args.K2)

    elif args.Problem == 3:
        from gen_pd_spir import gen_pd_spir
        fn = gen_pd_spir(args.N3, args.K3)
        xlabel = "Storage"
        ylabel = "Download"
        title = "Symmetric Private Information Retrieval with {} servers and {} files".format(args.N3, args.K3)
        name = "SPIR{}x{}".format(args.N3, args.K3)
    
    # Solve PD
    print()
    print("Solve the convex hull")
    if not os.path.exists(cai + 'PlotTradeoff/output'):
        os.makedirs(cai + 'PlotTradeoff/output')
    print('Open ' + cai + 'PlotTradeoff/output/Hull_' + name + '.txt for details')
    if os.path.exists(cai + 'PlotTradeoff/output/Hull_' + name + '.txt') and os.path.exists(cai + 'PlotTradeoff/output/Fig_' + name + '.eps'):
        print("file " + cai + "PlotTradeoff/output/Hull_" + name + ".txt already exists")
        print("Overwrite[y/n]:", end="")
        if input() == "y":
            pass
        else:
            with open(cai + 'PlotTradeoff/output/Hull_' + name + '.txt', 'w') as fout:
                subprocess.run([SOLVER, fn, "hull"], stdout=fout, text=True)
            """
            if args.Solver == 0:
                subprocess.run([cai + "CplexCompute/cplexcompute.out", fn, "hull"], stdout=fout, text=True)
            else:
                subprocess.run([cai + "GurobiCompute/gurobicompute.out", fn, "hull"], stdout=fout, text=True)
            """
    with open(cai + 'PlotTradeoff/output/Hull_' + name + '.txt', 'r') as fout:
        res = fout.read()

    # capture the points on the hull
    res = res[res.find("List of found points on the hull:\n"):-1].split("\n")[1: -1]
    points = []
    for p in res:
        points.append(tuple(map(float, p[1: -2].split(', '))))
    
    # plot region
    points = sorted(points, key=lambda x: x[0])
    width = points[0][1] - points[-1][1]
    plt.plot(*zip(*points), label = "Outer Bounds")
    if args.InPt != None:
        InPt = []
        for p in args.InPt.split(";"):
            InPt.append(tuple(map(float, p[1: -1].split(','))))
        plt.plot(*zip(*InPt), 'o', label = "Achievable Points")
    plt.ylim(points[-1][1]- 0.01 * width, points[0][1] + 0.01 * width)
    plt.ylabel(ylabel)
    plt.xlabel(xlabel)
    plt.title(title)
    plt.savefig(cai + 'PlotTradeoff/output/Fig_' + name + '.eps', format='eps')
    plt.legend()
    plt.show(block=False)
    plt.pause(PAUSE)
    plt.close()

    print("Figure " + cai + "PlotTradeoff/output/Fig_" + name + '.eps')