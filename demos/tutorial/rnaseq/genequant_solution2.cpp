#include <iostream>
#include <seqan/store.h>
#include <seqan/arg_parse.h>
#include <seqan/misc/interval_tree.h>
#include <seqan/parallel.h>

using namespace seqan;


// define used types
typedef FragmentStore<>                         TStore;
typedef Value<TStore::TAnnotationStore>::Type   TAnnotation;
typedef TAnnotation::TId                        TId;
typedef TAnnotation::TPos                       TPos;
typedef IntervalAndCargo<TPos, TId>             TInterval;

// define options
struct Options
{
    std::string annotationFileName;
    std::string alignmentFileName;
};


//
// 1. Parse command line and fill Options object
//
ArgumentParser::ParseResult parseOptions(Options & options, int argc, char const * argv[])
{
    ArgumentParser parser("gene_quant");
    setShortDescription(parser, "A simple gene quantification tool");
    setVersion(parser, "1.0");
    setDate(parser, "Sep 2012");

    addArgument(parser, ArgParseArgument(ArgParseArgument::INPUT_FILE));
    addArgument(parser, ArgParseArgument(ArgParseArgument::INPUT_FILE));
    addUsageLine(parser, "[\\fIOPTIONS\\fP] <\\fIANNOTATION FILE\\fP> <\\fIREAD ALIGNMENT FILE\\fP>");

    // Parse command line
    ArgumentParser::ParseResult res = parse(parser, argc, argv);

    if (res == ArgumentParser::PARSE_OK)
    {
        // Extract option values
        getArgumentValue(options.annotationFileName, parser, 0);
        getArgumentValue(options.alignmentFileName, parser, 1);
    }

    return res;
}

//
// 2. Load annotations and alignments from files
//
bool loadFiles(TStore & store, Options const & options)
{
    BamFileIn alignmentFile;
    if (!open(alignmentFile, options.alignmentFileName.c_str()))
    {
        std::cerr << "Couldn't open alignment file " << options.alignmentFileName << std::endl;
        return false;
    }
    std::cerr << "Loading read alignments ..... " << std::flush;
    readRecords(store, alignmentFile);
    std::cerr << "[" << length(store.alignedReadStore) << "]" << std::endl;

    // load annotations
    GffFileIn annotationFile;
    if (!open(annotationFile, options.annotationFileName.c_str()))
    {
        std::cerr << "Couldn't open annotation file" << options.annotationFileName << std::endl;
        return false;
    }
    std::cerr << "Loading genome annotation ... " << std::flush;
    readRecords(store, annotationFile);
    std::cerr << "[" << length(store.annotationStore) << "]" << std::endl;

    return true;
}

//![solution]
//
// 3. Extract intervals from gene annotations (grouped by contigId)
//
void extractGeneIntervals(String<String<TInterval> > & intervals, TStore const & store)
{
    // extract intervals from gene annotations (grouped by contigId)
    resize(intervals, length(store.contigStore));

    Iterator<TStore const, AnnotationTree<> >::Type it = begin(store, AnnotationTree<>());

    if (!goDown(it))
        return;

    do
    {
        SEQAN_ASSERT_EQ(getType(it), "gene");

        TPos beginPos = getAnnotation(it).beginPos;
        TPos endPos = getAnnotation(it).endPos;
        TId contigId = getAnnotation(it).contigId;

        if (beginPos > endPos)
            std::swap(beginPos, endPos);

        // insert forward-strand interval of the gene and its annotation id
        appendValue(intervals[contigId], TInterval(beginPos, endPos, value(it)));
    }
    while (goRight(it));
}
//![solution]

//![main]
int main(int argc, char const * argv[])
{
    Options options;
    TStore store;
    String<String<TInterval> > intervals;

    ArgumentParser::ParseResult res = parseOptions(options, argc, argv);
    if (res != ArgumentParser::PARSE_OK)
        return res == ArgumentParser::PARSE_ERROR;

    if (!loadFiles(store, options))
        return 1;

    extractGeneIntervals(intervals, store);

    return 0;
}
//![main]
