echo "example 1"

../build/transport_catalogue.exe make_base s14_3_example_1_make_base.json
../build/transport_catalogue.exe process_requests s14_3_example_1_process_requests.json > s14_3_example_1_output.json

cat s14_3_example_1_answer.json | diff -uw - s14_3_example_1_output.json

echo "example 2"

../build/transport_catalogue.exe make_base s14_3_example_2_make_base.json
../build/transport_catalogue.exe process_requests s14_3_example_2_process_requests.json > s14_3_example_2_output.json

cat s14_3_example_2_answer.json | python -m json.tool | diff -uw - s14_3_example_2_output.json

echo "opentest 1"

../build/transport_catalogue.exe make_base s14_3_opentest_1_make_base.json
../build/transport_catalogue.exe process_requests s14_3_opentest_1_process_requests.json > s14_3_opentest_1_output.json

cat s14_3_opentest_1_answer.json | python -m json.tool | diff -uw - s14_3_opentest_1_output.json

echo "opentest 2"

../build/transport_catalogue.exe make_base s14_3_opentest_2_make_base.json
../build/transport_catalogue.exe process_requests s14_3_opentest_2_process_requests.json > s14_3_opentest_2_output.json

cat s14_3_opentest_2_answer.json | python -m json.tool | diff -uw - s14_3_opentest_2_output.json

echo "opentest 3"

../build/transport_catalogue.exe make_base s14_3_opentest_3_make_base.json
../build/transport_catalogue.exe process_requests s14_3_opentest_3_process_requests.json > s14_3_opentest_3_output.json

cat s14_3_opentest_3_answer.json | python -m json.tool | diff -uw - s14_3_opentest_3_output.json
