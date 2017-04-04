#include <bitcoin/bitcoin.hpp>
#include <bitcoin/client.hpp>
#include "HD_walletTESTNET.cpp"
#include <string.h>

using namespace bc;
using namespace bc::wallet;
using namespace bc::machine;
using namespace bc::chain;

class channelReciever
{
public:
	void channelReciever(HD_Wallet wallet, int HDindex, ec_public payerKey)
	{
		payerKey = to_chunk(payerPublic.point());
		recieverKey = to_chunk(wallet.childPublicKey(HDindex).point());
		

	}

	void signRefund()
	{
		endorsement sig;
		script().create_endorsement(sig, wallet.childPrivateKey(HDindex).secret(), redeemScript, refund, 0, all);
		operation::list ops {operation(sig)};
		refund.inputs()[0].set_script(script(ops));
	}
	void setRedeem(script redeem))
	{
		redeemScript = redeem; 
	}
	void setBond(transaction bondTX)
	{
		if(refund.inputs()[0].previous_output().hash() == bondTX.hash()){
			bond = bondTX;
		}
		
	}
	void setRefund(transaction refundTX)
	{
		uint32_t now = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
		now = now + 85400;
		if(refund.locktime() >= now)
		{
			refund = refundTX;
		}
		signRefund();

	}

	points_value getOutputs(payment_address address, uint64_t amount)
	{
		client::obelisk_client client(connection);

		points_value val1;
		static const auto on_done = [&val1](const points_value& vals) {

			std::cout << "Success: " << vals.value() << std::endl;
			val1 = vals;
			

		};

		static const auto on_error = [](const code& ec) {

			std::cout << "Error Code: " << ec.message() << std::endl;

		};

		if(!client.connect(connection))
		{
			std::cout << "Fail" << std::endl;
		} else {
			std::cout << "Connection Succeeded" << std::endl;
		}

		client.blockchain_fetch_unspent_outputs(on_error, on_done, address, amount, select_outputs::algorithm::greedy);
		
		client.wait();
		
		
		//return allPoints;
		return val1;
	}

private:
	data_chunk recieverKey;
	data_chunk payerKey;

	ec_public payerPublic;
	HD_Wallet wallet; 
	script redeemScript;

	payment_address multisigAddy;
	payment_address payerAddy;
	payment_address recieverAddy;

	client::connection_type connection;
	int HDindex;
	uint64_t bondAmount;
	transaction bond;
	transaction refund;
}