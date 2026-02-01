#include "order_book_parser.h"

int main()
{
	cl::data_feed::data_feed_parser::SeekerNetBoonSnapshotParserToTBT parser({ 1,2 });
	parser.PrintFullBook(1);

	std::vector<cl::data_feed::data_feed_parser::bookElement> newBuyBook;
	{
		cl::data_feed::data_feed_parser::bookElement tmp;
		tmp.price = 50;
		tmp.qty = 40;
		tmp.time = std::time(0);
		newBuyBook.push_back(tmp);
	}
	{
		cl::data_feed::data_feed_parser::bookElement tmp;
		tmp.price = 30;
		tmp.qty = 451;
		tmp.time = std::time(0);
		newBuyBook.push_back(tmp);
	}
	parser.EmitOrdersAndUpdateOldBuyBook(1, newBuyBook, std::time(0));

	parser.PrintFullBook(1);

	parser.EmitMarketOrderAndUpdateBuyBook(1, 100, 50, std::time(0));

	parser.PrintFullBook(1);

	newBuyBook.clear();
	{
		cl::data_feed::data_feed_parser::bookElement tmp;
		tmp.price = 50;
		tmp.qty = 40;
		tmp.time = std::time(0);
		newBuyBook.push_back(tmp);
	}
	{
		cl::data_feed::data_feed_parser::bookElement tmp;
		tmp.price = 30;
		tmp.qty = 451;
		tmp.time = std::time(0);
		newBuyBook.push_back(tmp);
	}
	parser.EmitOrdersAndUpdateOldBuyBook(1, newBuyBook, std::time(0));

	parser.PrintFullBook(1);

}
