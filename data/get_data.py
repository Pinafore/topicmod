
from nltk import downloader

if __name__ == "__main__":
    for ii in ["punkt", "stopwords", "wordnet"]:
        downloader.download(ii)
