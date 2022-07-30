../build/transport_catalogue.exe make_base s14_2_opentest_1_make_base.json
../build/transport_catalogue.exe process_requests s14_2_opentest_1_process_requests.json > s14_2_opentest_1_output.json

diff -uw s14_2_opentest_1_answer.json s14_2_opentest_1_output.json

../build/transport_catalogue.exe make_base s14_2_opentest_2_make_base.json
../build/transport_catalogue.exe process_requests s14_2_opentest_2_process_requests.json > s14_2_opentest_2_output.json

diff -uw s14_2_opentest_2_answer.json s14_2_opentest_2_output.json

../build/transport_catalogue.exe make_base s14_2_opentest_3_make_base.json
../build/transport_catalogue.exe process_requests s14_2_opentest_3_process_requests.json > s14_2_opentest_3_output.json

diff -uw s14_2_opentest_3_answer.json s14_2_opentest_3_output.json
