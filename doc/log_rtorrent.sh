#!/bin/bash
args=($@)
echo "${args[@]:1}" >> $1
