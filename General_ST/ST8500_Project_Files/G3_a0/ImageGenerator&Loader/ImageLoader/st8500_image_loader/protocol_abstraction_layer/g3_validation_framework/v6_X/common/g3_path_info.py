#!/usr/bin/python3
try:
    import sys
    import os
    import pickle
    import argparse
    import matplotlib.pyplot as plt
    import networkx as nx
    import time
    import graphviz as gpz
except:
    print("Error importing libraries")
    raise ImportError

class g3_hop_info(object):
    def __init__(self, addr, link_cost):
        self.addr = int(addr)
        self.link_cost = int(link_cost)

class g3_device_path_info(object):
    def __init__(self, orig_addr, dest_addr, metric_type, hops_info_cnt, hops_info):
        self.orig_addr = int(orig_addr)
        self.dest_addr = int(dest_addr)
        self.metric_type = int(metric_type)
        self.hops_info_cnt = int(hops_info_cnt)
        self.hops_info = hops_info

class g3_path(object):
    def __init__(self, req_src, req_dst, resp_src, hops_infos):
        self.req_src = req_src
        self.req_dst = req_dst
        self.resp_src = resp_src
        self.dest_reached = True if (req_dst == resp_src) else False
        self.fwd_path = []
        self.bck_path = []
        self.fwd_path.append((req_src, 0))
        self.bck_path.append((resp_src, 0))
        fwd_ended = False
        for hop in hops_infos:
            if (not fwd_ended):
                self.fwd_path.append((hop.addr, hop.link_cost))
                if (hop.addr == resp_src):
                    fwd_ended = True
            else:
                self.bck_path.append((hop.addr, hop.link_cost))

    def edges_fwd(self):
        edges = []
        for i in range(1, len(self.fwd_path)):
            edges.append((self.fwd_path[i - 1][0], self.fwd_path[i][0], self.fwd_path[i][1]))
        return edges

    def edges_bck(self):
        edges = []
        for i in range(1, len(self.bck_path)):
            edges.append((self.bck_path[i - 1][0], self.bck_path[i][0], self.bck_path[i][1]))
        return edges        

def serialize_topology_info(topology_info_list, file_name):
    with open(file_name, "wb") as pickle_file:
        pickle.dump(topology_info_list, pickle_file)        

def get_hex_list_path(paths):
    hex_paths = []
    for path in paths:
        hex_path = []
        for num in path:
            hex_path.append('{0:02X}'.format(num))
        hex_paths.append(hex_path)
    return hex_paths        

g3_graph_type = ["circular", "kamada", "spring"]

def g3_generate_graph(graph_type, pkl_file="g3_topology_info_list.pkl", filename_suffix="graph", filename_prefix=""):
    network_paths = []
    nodes_list = []
    edges_fwd = []
    edges_bck = []
    digraph_fwd = nx.DiGraph()
    digraph_bck = nx.DiGraph()
    graph_fwd = nx.Graph()
    graph_bck = nx.Graph()
    labels = {}
    colors = []
    n_size = 300
    f_size = 12
    color_map = {0 : "yellow",
                 1 : "red", 2 : "blue", 3 : "green", 4 : "purple", 5 : "orange", 6 : "grey",
                 7 : "red", 8 : "blue", 9 : "green",10 : "purple",11 : "orange",12 : "grey",
                 13: "red",14 : "blue",15 : "green",16 : "purple",17 : "orange",18 : "grey",
                 }

    with open(pkl_file, 'rb') as pickle_file:
        topology_info_list = pickle.load(pickle_file)
    
    # coordinator has no hops
    colors.append(color_map.get(0))
    nodes_list.append(0)
    for info in topology_info_list:
        network_paths.append(g3_path(info.orig_addr, info.dest_addr, info.dest_addr, info.hops_info))
        nodes_list.append(info.dest_addr)
        num_hops = int(info.hops_info_cnt/2)
        colors.append(color_map.get(num_hops))

    if (len(network_paths) < 10):
        n_size = n_size * 10
        f_size = f_size * 2
    elif (len(network_paths) < 100):
        n_size = int(round(100 / len(network_paths)) * n_size)

    for path in network_paths:
        p_fwd = path.edges_fwd()
        p_bck = path.edges_bck()
        for p in p_fwd:
            edges_fwd.append(p)
        for p in p_bck:
            edges_bck.append(p)

    digraph_fwd.add_weighted_edges_from(edges_fwd)
    digraph_bck.add_weighted_edges_from(edges_bck)

    graph_fwd.add_weighted_edges_from(edges_fwd)
    graph_bck.add_weighted_edges_from(edges_bck)

    for node in graph_fwd.nodes():
        labels[node] = '{0:02X}'.format(node)

    if g3_graph_type[0] == graph_type:
        fig = plt.gcf()
        fig.set_size_inches(37, 21)

        pos_fwd = nx.circular_layout(digraph_fwd)
        nx.draw_networkx_nodes(digraph_fwd, pos_fwd)
        nx.draw_networkx_edges(digraph_fwd, pos_fwd)
        nx.draw_networkx_labels(digraph_fwd, pos_fwd, labels)

        plt.axis('off')
        plt.savefig(filename_prefix+"circular_fwd_"+filename_suffix+".png", dpi = 300)
        plt.close()
        
        fig = plt.gcf()
        fig.set_size_inches(37, 21)

        pos_bck = nx.circular_layout(digraph_bck)
        nx.draw_networkx_nodes(digraph_bck, pos_bck)
        nx.draw_networkx_edges(digraph_bck, pos_bck)
        nx.draw_networkx_labels(digraph_bck, pos_bck, labels)

        plt.axis('off')
        plt.savefig(filename_prefix+"circular_bck_"+filename_suffix+".png", dpi = 300)
        plt.close()
    elif g3_graph_type[1] == graph_type:    
        fig = plt.gcf()
        fig.set_size_inches(37, 21)

        pos_fwd = nx.kamada_kawai_layout(digraph_fwd, pos = nx.spectral_layout(digraph_fwd, scale=5))
        nx.draw_networkx_nodes(G=digraph_fwd, nodelist=nodes_list, pos=pos_fwd, node_size=n_size, node_color=colors)
        nx.draw_networkx_edges(digraph_fwd, pos_fwd, arrows = True)
        nx.draw_networkx_labels(G=digraph_fwd, pos=pos_fwd, labels=labels, font_size=f_size)

        plt.axis('off')
        plt.savefig(filename_prefix+"kamada_kawai_fwd_"+filename_suffix+".png", dpi = 300)
        plt.close()
        
        fig = plt.gcf()
        fig.set_size_inches(37, 21)

        pos_bck = nx.kamada_kawai_layout(digraph_bck, pos = nx.spectral_layout(digraph_bck, scale=5))
        nx.draw_networkx_nodes(G=digraph_bck, nodelist=nodes_list, pos=pos_bck, node_size=n_size, node_color=colors)
        nx.draw_networkx_edges(digraph_bck, pos_bck, arrows = True)
        nx.draw_networkx_labels(G=digraph_bck, pos=pos_bck, labels=labels, font_size=f_size)

        plt.axis('off')
        plt.savefig(filename_prefix+"kamada_kawai_bck_"+filename_suffix+".png", dpi = 300)
        plt.close()

    elif g3_graph_type[2] == graph_type:
        fig = plt.gcf()
        fig.set_size_inches(37, 21)

        pos_fwd = nx.spring_layout(digraph_fwd, k = 5)
        nx.draw_networkx_nodes(digraph_fwd, pos_fwd)
        nx.draw_networkx_edges(digraph_fwd, pos_fwd)
        nx.draw_networkx_labels(digraph_fwd, pos_fwd, labels)

        plt.axis('off')
        plt.savefig(filename_prefix+"spring_fwd_"+filename_suffix+".png", dpi = 300)
        plt.close()
        
        fig = plt.gcf()
        fig.set_size_inches(37, 21)

        pos_bck = nx.spring_layout(digraph_bck, k = 5)
        nx.draw_networkx_nodes(digraph_bck, pos_bck)
        nx.draw_networkx_edges(digraph_bck, pos_bck)
        nx.draw_networkx_labels(digraph_bck, pos_bck, labels)

        plt.axis('off')
        plt.savefig(filename_prefix+"spring_bck_"+filename_suffix+".png", dpi = 300)
        plt.close()
    else:
        print("Error in graph type selection:", g3_graph_type)
        return

    if (len(list(nx.cycle_basis(graph_fwd, 0))) > 0):
        print("WARNING: cycles in undirected graph_fwd: " + repr(get_hex_list_path(list(nx.cycle_basis(graph_fwd, 0)))))

    if (len(list(nx.cycle_basis(graph_bck, 0))) > 0):
        print("WARNING: cycles in undirected graph_bck: " + repr(get_hex_list_path(list(nx.cycle_basis(graph_bck, 0)))))

    if (len(list(nx.simple_cycles(digraph_fwd))) > 0):
        print("ERROR: cycles in digraph_fwd: " + repr(get_hex_list_path(list(nx.simple_cycles(digraph_fwd)))))

    if (len(list(nx.simple_cycles(digraph_bck))) > 0):
        print("ERROR: cycles in digraph_bck: " + repr(get_hex_list_path(list(nx.simple_cycles(digraph_bck)))))

def g3_generate_graph_gv(pkl_file="g3_topology_info_list.pkl", repeated_edges=False,
    filename_suffix="graph", filename_prefix=""):
    network_paths = []
    nodes_list = []
    digraph_fwd = gpz.Digraph()
    digraph_bck = gpz.Digraph()

    with open(pkl_file, 'rb') as pickle_file:
        topology_info_list = pickle.load(pickle_file)

    # coordinator has no hops
    nodes_list.append(0)
    for info in topology_info_list:
        network_paths.append(g3_path(info.orig_addr, info.dest_addr, info.dest_addr, info.hops_info))
        nodes_list.append(info.dest_addr)

    if repeated_edges:
        edges_fwd = []
        edges_bck = []
        for path in network_paths:
            p_fwd = path.edges_fwd()
            p_bck = path.edges_bck()
            for p in p_fwd:
                edges_fwd.append(("{0:03X}".format(p[0]), "{0:03X}".format(p[1])))
            for p in p_bck:
                edges_bck.append(("{0:03X}".format(p[0]), "{0:03X}".format(p[1])))
    else:
        edges_fwd = set()
        edges_bck = set()
        for path in network_paths:
            p_fwd = path.edges_fwd()
            p_bck = path.edges_bck()
            for p in p_fwd:
                edges_fwd.add(("{0:03X}".format(p[0]), "{0:03X}".format(p[1])))
            for p in p_bck:
                edges_bck.add(("{0:03X}".format(p[0]), "{0:03X}".format(p[1])))

    digraph_fwd.edges(edges_fwd)
    digraph_bck.edges(edges_bck)

    digraph_fwd.view(filename_prefix + "fwd_" + filename_suffix, cleanup=True)
    digraph_bck.view(filename_prefix + "bck_" + filename_suffix, cleanup=True)

if __name__ == "__main__":

    parser = argparse.ArgumentParser()
    parser.add_argument("--graph", help = "graph type to create, accepted values = " + repr(g3_graph_type))
    parser.add_argument("--pkl_file", help = "pkl filename that contains topology info (e.g. topology_info_list.pkl)", default="topology_info_list.pkl")
    args = parser.parse_args()

    g3_generate_graph_gv(pkl_file=args.pkl_file, filename_prefix=str(args.pkl_file).rstrip(".pkl")+"_")
