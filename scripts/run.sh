exec_file=./exec.out
if [ "$1" = "debug" ]; then
    exec_file=./exec_debug.out
fi
echo -e "exec_file: ${exec_file}"
echo -e "\nsoc-Epinions"
# ${exec_file} --data_dirpath ${PWD}/data/epinions --query_filepath ${PWD}/query/Q1_n.txt --method generic --options cache=on
# ${exec_file} --data_dirpath ${PWD}/data/epinions --query_filepath ${PWD}/query/Q2_n.txt --method generic --options cache=on
# ${exec_file} --data_dirpath ${PWD}/data/epinions --query_filepath ${PWD}/query/Q3_n.txt --method generic --options cache=on
# ${exec_file} --data_dirpath ${PWD}/data/epinions --query_filepath ${PWD}/query/Q4_n.txt --method generic --options cache=on
# ${exec_file} --data_dirpath ${PWD}/data/epinions --query_filepath ${PWD}/query/Q5_n.txt --method generic --options cache=on
# ${exec_file} --data_dirpath ${PWD}/data/epinions --query_filepath ${PWD}/query/Q6_n.txt --method generic --options cache=on
# ${exec_file} --data_dirpath ${PWD}/data/epinions --query_filepath ${PWD}/query/Q7_n.txt --method generic --options cache=on
echo -e "\nsoc-Slashdot"
# ${exec_file} --data_dirpath ${PWD}/data/slashdot --query_filepath ${PWD}/query/Q1_n.txt --method generic --options cache=on
# ${exec_file} --data_dirpath ${PWD}/data/slashdot --query_filepath ${PWD}/query/Q2_n.txt --method generic --options cache=on
# ${exec_file} --data_dirpath ${PWD}/data/slashdot --query_filepath ${PWD}/query/Q3_n_slashdot.txt --method generic --options cache=on
# ${exec_file} --data_dirpath ${PWD}/data/slashdot --query_filepath ${PWD}/query/Q4_n.txt --method generic --options cache=on
# ${exec_file} --data_dirpath ${PWD}/data/slashdot --query_filepath ${PWD}/query/Q5_n.txt --method generic --options cache=on
# ${exec_file} --data_dirpath ${PWD}/data/slashdot --query_filepath ${PWD}/query/Q6_n.txt --method generic --options cache=on
# ${exec_file} --data_dirpath ${PWD}/data/slashdot --query_filepath ${PWD}/query/Q7_n.txt --method generic --options cache=on
echo -e "\nego-Twitter"
# ${exec_file} --data_dirpath ${PWD}/data/ego_twitter --query_filepath ${PWD}/query/Q1_n.txt --method generic --options cache=on
# ${exec_file} --data_dirpath ${PWD}/data/ego_twitter --query_filepath ${PWD}/query/Q2_n.txt --method generic --options cache=on
# ${exec_file} --data_dirpath ${PWD}/data/ego_twitter --query_filepath ${PWD}/query/Q3_n.txt --method generic --options cache=on
# ${exec_file} --data_dirpath ${PWD}/data/ego_twitter --query_filepath ${PWD}/query/Q4_n.txt --method generic --options cache=on
# ${exec_file} --data_dirpath ${PWD}/data/ego_twitter --query_filepath ${PWD}/query/Q5_n.txt --method generic --options cache=on
# ${exec_file} --data_dirpath ${PWD}/data/ego_twitter --query_filepath ${PWD}/query/Q6_n.txt --method generic --options cache=on
# ${exec_file} --data_dirpath ${PWD}/data/ego_twitter --query_filepath ${PWD}/query/Q7_n.txt --method generic --options cache=on
echo -e "\nAmazon"
# ${exec_file} --data_dirpath ${PWD}/data/amazon --query_filepath ${PWD}/query/Q1_n.txt --method generic --options cache=on
# ${exec_file} --data_dirpath ${PWD}/data/amazon --query_filepath ${PWD}/query/Q2_n.txt --method generic --options cache=on
# ${exec_file} --data_dirpath ${PWD}/data/amazon --query_filepath ${PWD}/query/Q3_n_amazon.txt --method generic --options cache=on
# ${exec_file} --data_dirpath ${PWD}/data/amazon --query_filepath ${PWD}/query/Q4_n.txt --method generic --options cache=on
# ${exec_file} --data_dirpath ${PWD}/data/amazon --query_filepath ${PWD}/query/Q5_n.txt --method generic --options cache=on
# ${exec_file} --data_dirpath ${PWD}/data/amazon --query_filepath ${PWD}/query/Q6_n_amazon.txt --method generic --options cache=on
# ${exec_file} --data_dirpath ${PWD}/data/amazon --query_filepath ${PWD}/query/Q7_n_amazon.txt --method generic --options cache=on
echo -e "\nweb-Google"
# ${exec_file} --data_dirpath ${PWD}/data/google --query_filepath ${PWD}/query/Q1_n.txt --method generic --options cache=on
# ${exec_file} --data_dirpath ${PWD}/data/google --query_filepath ${PWD}/query/Q2_n.txt --method generic --options cache=on
# ${exec_file} --data_dirpath ${PWD}/data/google --query_filepath ${PWD}/query/Q3_n.txt --method generic --options cache=on
# ${exec_file} --data_dirpath ${PWD}/data/google --query_filepath ${PWD}/query/Q4_n.txt --method generic --options cache=on
# ${exec_file} --data_dirpath ${PWD}/data/google --query_filepath ${PWD}/query/Q5_n_google.txt --method generic --options cache=on
# ${exec_file} --data_dirpath ${PWD}/data/google --query_filepath ${PWD}/query/Q6_n.txt --method generic --options cache=on
# ${exec_file} --data_dirpath ${PWD}/data/google --query_filepath ${PWD}/query/Q7_n.txt --method generic --options cache=on
echo -e "\nweb-BerkStan"
# ${exec_file} --data_dirpath ${PWD}/data/berkstan --query_filepath ${PWD}/query/Q1_n.txt --method generic --options cache=on
# ${exec_file} --data_dirpath ${PWD}/data/berkstan --query_filepath ${PWD}/query/Q2_n.txt --method generic --options cache=on
# ${exec_file} --data_dirpath ${PWD}/data/berkstan --query_filepath ${PWD}/query/Q3_n_amazon.txt --method generic --options cache=on
# ${exec_file} --data_dirpath ${PWD}/data/berkstan --query_filepath ${PWD}/query/Q4_n.txt --method generic --options cache=on
# ${exec_file} --data_dirpath ${PWD}/data/berkstan --query_filepath ${PWD}/query/Q5_n.txt --method generic --options cache=on
# ${exec_file} --data_dirpath ${PWD}/data/berkstan --query_filepath ${PWD}/query/Q6_n_amazon.txt --method generic --options cache=on
# ${exec_file} --data_dirpath ${PWD}/data/berkstan --query_filepath ${PWD}/query/Q7_n_amazon.txt --method generic --options cache=on
echo -e "\nwiki-topCats"
# ${exec_file} --data_dirpath ${PWD}/data/wiki-topcats --query_filepath ${PWD}/query/Q1_n.txt --method generic --options cache=on
# ${exec_file} --data_dirpath ${PWD}/data/wiki-topcats --query_filepath ${PWD}/query/Q2_n.txt --method generic --options cache=on
# ${exec_file} --data_dirpath ${PWD}/data/wiki-topcats --query_filepath ${PWD}/query/Q3_n.txt --method generic --options cache=on
# ${exec_file} --data_dirpath ${PWD}/data/wiki-topcats --query_filepath ${PWD}/query/Q4_n.txt --method generic --options cache=on
# ${exec_file} --data_dirpath ${PWD}/data/wiki-topcats --query_filepath ${PWD}/query/Q5_n_google.txt --method generic --options cache=on
# ${exec_file} --data_dirpath ${PWD}/data/wiki-topcats --query_filepath ${PWD}/query/Q6_n_wiki.txt --method generic --options cache=on
# ${exec_file} --data_dirpath ${PWD}/data/wiki-topcats --query_filepath ${PWD}/query/Q7_n_wiki.txt --method generic --options cache=on
echo -e "\ncit-Patents"
${exec_file} --data_dirpath ${PWD}/data/patent --query_filepath ${PWD}/query/Q1.txt --method generic --options cache=on
${exec_file} --data_dirpath ${PWD}/data/patent --query_filepath ${PWD}/query/Q2.txt --method generic --options cache=on
${exec_file} --data_dirpath ${PWD}/data/patent --query_filepath ${PWD}/query/Q3.txt --method generic --options cache=on
${exec_file} --data_dirpath ${PWD}/data/patent --query_filepath ${PWD}/query/Q4.txt --method generic --options cache=on
${exec_file} --data_dirpath ${PWD}/data/patent --query_filepath ${PWD}/query/Q5.txt --method generic --options cache=on
${exec_file} --data_dirpath ${PWD}/data/patent --query_filepath ${PWD}/query/Q6.txt --method generic --options cache=on
${exec_file} --data_dirpath ${PWD}/data/patent --query_filepath ${PWD}/query/Q7.txt --method generic --options cache=on