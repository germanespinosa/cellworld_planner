#!/bin/bash

for i in {0..44}; do
#     python .\Step_additional_info.py
    lzi=$(printf "%03d" $i)
    python /research/cellworld_planner/python/Step_additional_info.py /research/belief_state_results/episode_$i.json "/research/googledrive/MacIver Lab: Habitat/Trajectory Data/Videos/Musculus/Dreadds Experiments/MICE_20220608_1557_DMM3_21_05_SR5/episode_$lzi/main_MICE_20220608_1557_DMM3_21_05_SR5.mp4" /research/belief_state_results/episode_$lzi.mp4
done