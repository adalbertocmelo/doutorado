#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import sem
import argparse
import glob
from shutil import copytree

ns_path = './'
script = 'Fog4VR'
campaign_dir = ns_path + 'sem'
results_dir = ns_path + 'results'
nRuns = 5

parser = argparse.ArgumentParser(description='SEM script')
parser.add_argument('-o', '--overwrite', action='store_true',
                    help='Overwrite previous campaign')
args = parser.parse_args()

campaign = sem.CampaignManager.new(ns_path, script, campaign_dir,
            overwrite=args.overwrite, check_repo=False)

print("running simulations")
param_combinations = {
    'simulationId' : list(range(nRuns)),
    'numberOfClients' : [50, 100],
    'segmentDuration' : 40000,
    'adaptationAlgo' : 'festive',
    'segmentSizeFile' : 'src/Fog4VR/dash/segmentSizesBigBuckVR.txt',
    'seedValue' : 4200,
    'politica' : list(range(4,8))
}

campaign.run_missing_simulations(sem.list_param_combinations(param_combinations),
        runs=1, stop_on_errors=False)

print("exporting results")
runs = glob.glob(campaign_dir + "/data/*/dash-log-files")
for run in runs:
    copytree(run, results_dir, dirs_exist_ok=True)
