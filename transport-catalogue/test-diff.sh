# set -o noexec
set -o nounset
set -o verbose

diff -ub -U 20 --color=always data/s12_e1_answer.json data/s12_e1_output.json
diff -ub -U 20 --color=always data/s12_e2_answer.json data/s12_e2_output.json
diff -ub -U 20 --color=always data/s12_e3_answer.json data/s12_e3_output.json
diff -ub -U 20 --color=always data/s12_e4_answer.json data/s12_e4_output.json
diff -ub -U 20 --color=always data/s12_final_opentest_1_answer.json data/s12_final_opentest_1_output.json
diff -ub -U 20 --color=always data/s12_final_opentest_2_answer.json data/s12_final_opentest_2_output.json
diff -ub -U 20 --color=always data/s12_final_opentest_3_answer.json data/s12_final_opentest_3_output.json
