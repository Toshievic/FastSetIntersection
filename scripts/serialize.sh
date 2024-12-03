export DATASET_BASE_PATH=/Users/tito/research/data/dwh/labeled/
exec_file=./serialize.out
if [ "$1" = "debug" ]; then
    exec_file=./serialize_debug.out
fi
echo -e "exec_file: ${exec_file}"
echo -e "\nsoc-Epinions"
rm -r ${PWD}/data/epinions
${exec_file} --input_dirpath ${DATASET_BASE_PATH}/epinions --output_dirpath ${PWD}/data/epinions
echo -e "\nsoc-Slashdot"
rm -r ${PWD}/data/slashdot
${exec_file} --input_dirpath ${DATASET_BASE_PATH}/slashdot --output_dirpath ${PWD}/data/slashdot
echo -e "\nego-Twitter"
rm -r ${PWD}/data/ego_twitter
${exec_file} --input_dirpath ${DATASET_BASE_PATH}/ego_twitter --output_dirpath ${PWD}/data/ego_twitter
echo -e "\nAmazon"
rm -r ${PWD}/data/amazon
${exec_file} --input_dirpath ${DATASET_BASE_PATH}/amazon --output_dirpath ${PWD}/data/amazon
echo -e "\nweb-Google"
rm -r ${PWD}/data/google
${exec_file} --input_dirpath ${DATASET_BASE_PATH}/google --output_dirpath ${PWD}/data/google
echo -e "\nweb-BerkStan"
rm -r ${PWD}/data/berkstan
${exec_file} --input_dirpath ${DATASET_BASE_PATH}/berkstan --output_dirpath ${PWD}/data/berkstan
echo -e "\nwiki-topCats"
rm -r ${PWD}/data/wiki-topcats
${exec_file} --input_dirpath ${DATASET_BASE_PATH}/wiki-topcats --output_dirpath ${PWD}/data/wiki-topcats
echo -e "\ncit-Patents"
rm -r ${PWD}/data/patent
${exec_file} --input_dirpath ${DATASET_BASE_PATH}/patent --output_dirpath ${PWD}/data/patent
