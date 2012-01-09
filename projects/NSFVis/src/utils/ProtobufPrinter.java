package utils;

import java.io.FileInputStream;

import lib_corpora_proto.ProtoCorpus.Corpus;
import lib_corpora_proto.ProtoCorpus.Document;
import lib_corpora_proto.ProtoCorpus.Document.Sentence;
import lib_corpora_proto.ProtoCorpus.Document.Sentence.Word;
import lib_corpora_proto.ProtoCorpus.Vocab;

/**
 * Prints out document information from a protocol buffer. This is for 
 * debugging.
 */
public class ProtobufPrinter
{
	public static void main(String[] args) throws Exception
	{
		//String indexLoc = "C:/corpus/nsfvis/nsfSmall/protocol/nsfSmall.index";
		//String fileLoc = "C:/corpus/nsfvis/nsfSmall/protocol/0723468";
		
		String indexLoc = "C:/corpus/nsfvis/model_topic_assign/doc_voc.index";
		String fileLoc = "C:/corpus/nsfvis/model_topic_assign/assign/0956632";
		
		Corpus corpus = Corpus.parseFrom(new FileInputStream(indexLoc));
		Document doc = Document.parseFrom(new FileInputStream(fileLoc));
		
		Vocab vocab = corpus.getLemmas(0);
		Sentence sent = doc.getSentences(0);
		for (Word word : sent.getWordsList())
		{
			int index = word.getLemma();
			String term = vocab.getTerms(index).getOriginal();
			System.out.println(index + " " + term);
		}
	}
}
