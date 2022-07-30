../build/transport_catalogue.exe make_base s14_1_opentest_1_make_base.json
../build/transport_catalogue.exe process_requests s14_1_opentest_1_process_requests.json > s14_1_opentest_1_output.json

diff -uw s14_1_opentest_1_answer.json s14_1_opentest_1_output.json

../build/transport_catalogue.exe make_base s14_1_opentest_2_make_base.json
../build/transport_catalogue.exe process_requests s14_1_opentest_2_process_requests.json > s14_1_opentest_2_output.json

diff -uw s14_1_opentest_2_answer.json s14_1_opentest_2_output.json

../build/transport_catalogue.exe make_base s14_1_opentest_3_make_base.json
../build/transport_catalogue.exe process_requests s14_1_opentest_3_process_requests.json > s14_1_opentest_3_output.json

diff -uw s14_1_opentest_3_answer.json s14_1_opentest_3_output.json

../build/transport_catalogue.exe make_base s14_e1_make_base.json
../build/transport_catalogue.exe process_requests s14_e1_process_request.json > s14_e1_output.json

diff -uw s14_e1_answer.json s14_e1_output.json

../build/transport_catalogue.exe make_base s14_e2_make_base.json
../build/transport_catalogue.exe process_requests s14_e2_process_request.json > s14_e2_output.json

diff -uw s14_e2_answer.json s14_e2_output.json

../build/transport_catalogue.exe make_base s14_e3_make_base.json
../build/transport_catalogue.exe process_requests s14_e3_process_request.json > s14_e3_output.json

diff -uw s14_e3_answer.json s14_e3_output.json

../build/transport_catalogue.exe make_base s14_e4_make_base.json
../build/transport_catalogue.exe process_requests s14_e4_process_request.json > s14_e4_output.json

diff -uw s14_e4_answer.json s14_e4_output.json
