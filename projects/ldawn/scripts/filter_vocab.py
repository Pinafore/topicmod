from nltk import FreqDist
from PMI_statistics import get_tfidf

def sort_tfidf(infilename, outfilename):
  inputfile = open(infilename)
  outputfile = open(outfilename, 'w')

  freqdist = FreqDist()
  for line in inputfile:
    line = line.strip()
    words = line.split("\t")
    freqdist[words[0]] = float(words[1])

  for word in freqdist.keys():
    tmp = word + "\t" + str(freqdist[word]) + "\n"
    outputfile.write(tmp)

  inputfile.close()
  outputfile.close()

def filterVocab(infilename, outfilename, thresh):
  inputfile = open(infilename)
  outputfile = open(outfilename, 'w')

  freqdist = FreqDist()
  for line in inputfile:
    line = line.strip()
    words = line.split("\t")
    freqdist[words[0]] = float(words[1])

  count = 0
  for word in freqdist.keys():
    count += 1
    if count <= thresh:
      tmp = "0" + "\t" + word + "\n"
      outputfile.write(tmp)

  inputfile.close()
  outputfile.close()


def get_20news_tfidf():
  vocab_file = "vocab/20_news_stem_all.voc"
  proto_corpus_dir = "output/20_news_stem_tfidf/iter_1_all/model_topic_assign/"
  #proto_corpus_dir = "../../data/20_news_date/numeric"
  output_dir = "PMI_stat/20_news_stem_tfidf/"
  get_tfidf(proto_corpus_dir, vocab_file, output_dir)


def nyt_tfidf():
  vocab_file = "vocab/nyt.voc"
  proto_corpus_dir = "output/nyt/iter_1_all/model_topic_assign/"
  output_dir = "PMI_stat/nyt/"
  get_tfidf(proto_corpus_dir, vocab_file, output_dir)
  

if __name__ == "__main__":

  option = 0

  if option == 0:
    # 20 news:
    get_20news_tfidf()
    infilename = "PMI_stat/20_news_stem_tfidf/tfidf_mean.txt"
    outfilename = "PMI_stat/20_news_stem_tfidf/tfidf_mean.txt.sorted"
    sort_tfidf(infilename, outfilename)
    outfilename = "vocab/20_news_stem_tfidf.voc"
    thresh = 10000 # 0.018
    filterVocab(infilename, outfilename, thresh)

  elif option == 1:
    # nyt
    nyt_tfidf()
    infilename = "PMI_stat/nyt/tfidf_mean.txt"
    outfilename = "PMI_stat/nyt/tfidf_mean.txt.sorted"
    sort_tfidf(infilename, outfilename)
    outfilename = "vocab/nyt_sv.voc"
    thresh = 5000
    filterVocab(infilename, outfilename, thresh)

