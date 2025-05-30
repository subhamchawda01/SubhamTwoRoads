import sys


def main():
    Usage = "USAGE: " + \
        sys.argv[0] + " INPUT_MODEL1 [INPUT_MODEL2] [INPUT_MODEL3].... OUTPUT_MODEL_FILE METHOD [ CUMULATIVE/SELECTIVE ]"
    if (len(sys.argv)) < 4:
        print Usage
        return

    input_models_ = []
    i = 1
    while (i < len(sys.argv) - 2):
        input_models_.append(sys.argv[i])
        i = i + 1
    num_models_ = i - 1
    output_model_ = sys.argv[len(sys.argv) - 2]
    method_ = sys.argv[len(sys.argv) - 1]

    out_file_ = open(output_model_, "w")
    i = 0
    model_start_ = []
    model_end_ = []
    indicators_ = []
    count_ = 0
    if 'SELECTIVE' in method_:
        model_init_written_ = False
        model_math_written_ = False
        indicator_start_written_ = False
        write_empty_model_ = False
        model_content_ = []
        while (count_ < len(input_models_)):
            model_file_not_found_ = False
            model_file_ = input_models_[count_]
            try:
                model_content_ = open(model_file_).readlines()
            except IOError:
                model_file_not_found_ = True

            if (len(model_content_) < 4 or model_file_not_found_) and model_init_written_ and model_math_written_ and indicator_start_written_:
                out_file_.write('INDICATORINTERMEDIATE\n')
                if ((len(input_models_) - 1) == count_):
                    out_file_.write('INDICATOREND\n')
                count_ = count_ + 1
                continue
#      elif ( len ( model_content_ ) < 4 ) :
#        write_empty_model_ = True

            for line in model_content_:
                #        if ( model_init_written_ and model_math_written_ and write_empty_model_  and indicator_start_written_ ) :
                #          out_file_.write( "INDICATORINTERMEDIATE\n")
                #          write_empty_model_ = False

                if 'MODELINIT' in line and not model_init_written_:
                    out_file_.write(line.strip() + "\n")
                    model_init_written_ = True
                if 'MODELMATH' in line and not model_math_written_:
                    line_words_ = line.strip().split()
                    line_words_[1] = 'SELECTIVE'
                    out_file_.write(' '.join(line_words_) + "\n")
                    model_math_written_ = True
                if 'INDICATORSTART' in line and not indicator_start_written_:
                    out_file_.write(line.strip() + "\n")
                    indicator_start_written_ = True
                if 'INDICATOR ' in line:
                    out_file_.write(line.strip() + "\n")

                if 'INDICATOREND' in line:
                    line = line.strip().split('#')
                    if (len(line) >= 2):
                        com_ = line[1]
                    elif (len(line) == 1):
                        com_ = " "
                    if count_ != len(input_models_) - 1:
                        out_file_.write('INDICATORINTERMEDIATE' + ' # ' + com_ + '\n')
                    else:
                        out_file_.write('INDICATOREND' + ' # ' + com_ + '\n')

            count_ = count_ + 1
        out_file_.close()
        return

        # If we just want to linerarly add the model
    for model_file_ in input_models_:
        file_not_found_ = False
        model_cont_ = []

        try:
            model_cont_ = open(model_file_).readlines()
        except IOError:
            file_not_found_ = True
        if not file_not_found_:
            model_start_ = []
            model_end_ = []
        for line in model_cont_:
            found = False
            if 'MODELINIT' in line:
                model_start_.append(line)
            if 'MODELMATH' in line:
                model_start_.append(line)
            if "INDICATORSTART" in line:
                model_start_.append(line)
            if "INDICATOREND" in line:
                model_end_.append(line)
            if "INDICATOR " in line:
                ind_cont_ = line.split('#')[0]
                ind_lin_words_ = ind_cont_.split()
                if (len(ind_lin_words_) < 3):
                    continue
                j = 0
                while (j < len(indicators_)):
                    if ind_lin_words_[2] in indicators_[j]:
                        print " #Exists : " + ind_cont_
                        print " Earlier Value: " + indicators_[j]
                        temp_line_ = indicators_[j].split()
                        temp_line_[1] = str(float(temp_line_[1]) + float(ind_lin_words_[1]) / float(num_models_))
                        indicators_[j] = ' '.join(temp_line_)
                        found = True
                    j = j + 1

                if not found:
                    ind_words_ = ind_cont_.split()
                    if (len(ind_words_) > 3):
                        ind_words_[1] = str(float(ind_words_[1]) / float(num_models_))
                        indicators_.append(' '.join(ind_words_))

    out_file_.write(''.join(model_start_))
    out_file_.write('\n'.join(indicators_) + '\n')
    out_file_.write('\n'.join(model_end_))
    out_file_.close()


if __name__ == "__main__":
    main()
