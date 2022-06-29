# set -o noexec
set -o nounset
set -o verbose

./transport.out data/s12_e1_input.json > data/s12_e1_output.json

./transport.out data/s12_e2_input.json > data/s12_e2_output.json

./transport.out data/s12_e3_input.json > data/s12_e3_output.json

./transport.out data/s12_e4_input.json > data/s12_e4_output.json

./transport.out data/s12_final_opentest_1.json > data/s12_final_opentest_1_output.json

./transport.out data/s12_final_opentest_2.json > data/s12_final_opentest_2_output.json

./transport.out data/s12_final_opentest_3.json > data/s12_final_opentest_3_output.json
