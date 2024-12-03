exec_file=./exec.out
if [ "$1" = "debug" ]; then
    exec_file=./exec_debug.out
fi
echo -e "exec_file: ${exec_file}"
echo -e "\nsoc-Epinions"
${exec_file} --data_dirpath ${PWD}/data/epinions --query_filepath ${PWD}/query/Q1_n.txt --method generic --options switch=lazy
echo -e "\nsoc-Slashdot"
${exec_file} --data_dirpath ${PWD}/data/slashdot --query_filepath ${PWD}/query/Q1_n_slashdot.txt --method generic --options switch=lazy
echo -e "\nego-Twitter"
${exec_file} --data_dirpath ${PWD}/data/ego_twitter --query_filepath ${PWD}/query/Q1_n.txt --method generic --options switch=lazy
echo -e "\nAmazon"
${exec_file} --data_dirpath ${PWD}/data/amazon --query_filepath ${PWD}/query/Q1_n.txt --method generic --options switch=lazy
echo -e "\nweb-Google"
${exec_file} --data_dirpath ${PWD}/data/google --query_filepath ${PWD}/query/Q1_n.txt --method generic --options switch=lazy
echo -e "\nweb-BerkStan"
${exec_file} --data_dirpath ${PWD}/data/berkstan --query_filepath ${PWD}/query/Q1_n.txt --method generic --options switch=lazy
echo -e "\nwiki-topCats"
${exec_file} --data_dirpath ${PWD}/data/wiki-topcats --query_filepath ${PWD}/query/Q1_n.txt --method generic --options switch=lazy
echo -e "\ncit-Patents"
${exec_file} --data_dirpath ${PWD}/data/patent --query_filepath ${PWD}/query/Q1.txt --method generic --options switch=lazy