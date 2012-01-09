from topicmod.util import flags
import re
from numpy import zeros

flags.define_string("folder_base", "output/20_news/", \
                                  "Input file folder")
flags.define_string("output_base", "output/20_news/results_compare", \
                                  "Output file name")


if __name__ == "__main__":
  flags.InitFlags()

  # comparing 1

  filename = flags.output_base + 'results_compare_1.csv'
  outputfile = open(filename, 'w')

  folders = open(flags.folder_base + 'folders.txt', 'r')
  for folder in folders:
    folder = folder.strip()

    tmp = folder + '\n\n'
    outputfile.write(tmp)
    filename = folder.replace('results_', 'iter_100_')
    filename = flags.folder_base + folder + '/' + filename + '.txt'
    inputfile = open(filename, 'r')
    for line in inputfile:
      outputfile.write(line)
    inputfile.close()
    outputfile.write('\n\n\n')

    tmp = folder + '_ew\n\n'
    outputfile.write(tmp)
    filename = folder.replace('results_', 'iter_100_')
    filename = flags.folder_base + folder + '/' + filename + '_wiki.txt'
    inputfile = open(filename, 'r')
    for line in inputfile:
      outputfile.write(line)
    inputfile.close()
    outputfile.write('\n\n\n')

  folders.close()
  outputfile.close()



  # comparing 2
  results = dict()
  folders = open(flags.folder_base + 'folders.txt', 'r')
  for folder in folders:
    folder = folder.strip()

    filename = folder.replace('results_', 'iter_100_')
    filename = flags.folder_base + folder + '/' + filename + '.txt'
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

      tmp = words
      tmp[0] = folder
      results[word_key].append(tmp)    

    inputfile.close()

  folders.close()

  folders = open(flags.folder_base + 'folders.txt', 'r')
  for folder in folders:
    folder = folder.strip()

    filename = folder.replace('results_', 'iter_100_')
    filename = flags.folder_base + folder + '/' + filename + '_wiki.txt'
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

      tmp = words
      tmp[0] = folder + "_ew"
      results[word_key].append(tmp)

    inputfile.close()

  folders.close()

  filename = flags.output_base + 'results_compare_2.csv'
  outputfile = open(filename, 'w')

  for tt in results.keys():

    if tt == -1:
      tmp = 'total\n\n'
    else:
      tmp = 'Topic ' + str(tt) + '\n\n'

    outputfile.write(tmp)

    for res in results[tt]:
      tmp = "\t".join(res)
      tmp += "\n"
      outputfile.write(tmp)

    outputfile.write('\n\n\n')

  # stats 1.1
  compName = "^results_nocons$"
  standards = dict()
  for tt in results.keys(): 
    for res in results[tt]:
      if re.search(compName, res[0]):
        standards[tt] = []
        standards[tt]= res[1:]

  stats = dict()
  for tt in results.keys():
    if tt == -1:
      continue
    for res in results[tt]:
      folder = res[0]
      if folder not in stats.keys() and not re.search("_ew", folder):
        stats[folder] = zeros(len(res)-1)

      if folder in stats.keys():
        for ii in range(1, len(res)):        
          if res[ii] > standards[tt][ii-1]:
            stats[folder][ii-1] += 1

  tmp = "Compare with results_nocons (>)\n\n"
  outputfile.write(tmp)
  for folder in stats.keys():
    tmp = folder
    for count in stats[folder]:
      tmp += "\t" + str(count)
    tmp += "\n"
    outputfile.write(tmp)
  outputfile.write('\n\n\n')

  # stats 1.2
  compName = "^results_nocons$"
  standards = dict()
  for tt in results.keys(): 
    for res in results[tt]:
      if re.search(compName, res[0]):
        standards[tt] = []
        standards[tt]= res[1:]

  stats = dict()
  for tt in results.keys():
    if tt == -1:
      continue
    for res in results[tt]:
      folder = res[0]
      if folder not in stats.keys() and not re.search("_ew", folder):
        stats[folder] = zeros(len(res)-1)

      if folder in stats.keys():
        for ii in range(1, len(res)):        
          if res[ii] >= standards[tt][ii-1]:
            stats[folder][ii-1] += 1

  tmp = "Compare with results_nocons (>=)\n\n"
  outputfile.write(tmp)
  for folder in stats.keys():
    tmp = folder
    for count in stats[folder]:
      tmp += "\t" + str(count)
    tmp += "\n"
    outputfile.write(tmp)
  outputfile.write('\n\n\n')


  # stats 2.1
  compName = "^results_nocons_ew$"
  standards = dict()
  flag = 0
  for tt in results.keys(): 
    for res in results[tt]:
      if re.search(compName, res[0]):
        flag = 1
        standards[tt] = []
        standards[tt]= res[1:]

  if flag == 1:
    stats = dict()
    for tt in results.keys():
      if tt == -1:
        continue
      for res in results[tt]:
        folder = res[0]
        if folder not in stats.keys() and re.search("_ew", folder):
          stats[folder] = zeros(len(res)-1)

        if folder in stats.keys():
          for ii in range(1, len(res)):        
            if res[ii] > standards[tt][ii-1]:
              stats[folder][ii-1] += 1

    tmp = "Compare with results_nocons_ew (>)\n\n"
    outputfile.write(tmp)
    for folder in stats.keys():
      tmp = folder
      for count in stats[folder]:
        tmp += "\t" + str(count)
      tmp += "\n"
      outputfile.write(tmp)
    outputfile.write('\n\n\n')


  # stats 2.2
  compName = "^results_nocons_ew$"
  standards = dict()
  flag = 0
  for tt in results.keys(): 
    for res in results[tt]:
      if re.search(compName, res[0]):
        flag = 1
        standards[tt] = []
        standards[tt]= res[1:]

  if flag == 1:
    stats = dict()
    for tt in results.keys():
      if tt == -1:
        continue
      for res in results[tt]:
        folder = res[0]
        if folder not in stats.keys() and re.search("_ew", folder):
          stats[folder] = zeros(len(res)-1)

        if folder in stats.keys():
          for ii in range(1, len(res)):        
            if res[ii] >= standards[tt][ii-1]:
              stats[folder][ii-1] += 1

    tmp = "Compare with results_nocons_ew (>)\n\n"
    outputfile.write(tmp)
    for folder in stats.keys():
      tmp = folder
      for count in stats[folder]:
        tmp += "\t" + str(count)
      tmp += "\n"
      outputfile.write(tmp)
    outputfile.write('\n\n\n')


  outputfile.close()
