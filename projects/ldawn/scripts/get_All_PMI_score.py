from topicmod.util import flags

flags.define_string("input_base", "output/20_news/iter_100_PMI_", \
                                  "Input file folder")
flags.define_string("output_base", "output/20_news/iter_100_PMI", \
                                  "Output file name")
flags.define_string("PMI_file", "PMI_score", \
                                  "Output file name")
flags.define_int("round_num", "5", "Number of iteractive rounds")

if __name__ == "__main__":
  flags.InitFlags()

  results = dict()
  rounds = flags.round_num + 1
  for ii in range(0, rounds):
    filename = flags.input_base + str(ii) + "/" + flags.PMI_file
    inputfile = open(filename, 'r')
    for line in inputfile:
      line = line.strip()
      words = line.split('\t')
      if words[0].find('total') >= 0:
        word_key = -1
      else:
        word_key = int(words[0])
      if word_key not in results.keys():
        results[word_key] = []
      results[word_key].append(words[2])

  outputfile = open(flags.output_base, 'w')
  for tt in results.keys():
    if tt == -1:
      tmp = "total" + "\t"
    else:
      tmp = str(tt) + "\t"
    tmp += "\t".join(results[tt])
    tmp += "\n"
    outputfile.write(tmp)

  outputfile.close()
