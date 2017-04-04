#include <bitcoin/bitcoin.hpp>
#include <bitcoin/client.hpp>
#include "HD_walletTESTNET.cpp"
#include <string.h>

using namespace bc;
using namespace bc::wallet;
using namespace bc::machine;
using namespace bc::chain;

class channelPayer
{
public: 
	//constructor
	channelPayer(HD_Wallet wallet, int HDindex, ec_public recieverPublic, std::string amount)
	{
		recieverKey = to_chunk(recieverPublic.point());
		payerKey = to_chunk(wallet.childPublicKey(HDindex).point());
		bondAmount = btcToSatoshi(amount);

		payerAddy = wallet.childAddress(HDindex);
		recieverAddy = payment_address(recieverPublic);
		multisigAddy = makeMultisigAddress();

		connection = connectionSettings();

		bond = makeBond();

		refund = makeRefund();




	}

	script makeRedeemScript()
	{
		data_stack keys {payerKey, recieverKey};
		script multisig = script(script().to_pay_multisig_pattern(2, keys));
		return multisig;
	}

	payment_address makeMultisigAddress()
	{
		redeemScript = makeRedeemScript();
		return payment_address(redeemScript, 0xc4);
	}

	ec_public getPayee(std::string key)
	{
		return ec_public(key);
	}
	uint64_t btcToSatoshi(std::string amount)
	{

		uint64_t Satoshis; 
		decode_base10(Satoshis, amount, 8);
		return Satoshis;
	}
	output makeChange(uint64_t value)
	{
		uint64_t change = value - bondAmount - 1000;
		script outScript = script(script().to_pay_key_hash_pattern(payerAddy.hash()));
	
		std::cout << "change: "<< change << std::endl;
		return output(change, outScript);
	}

	transaction makeBond()
	{

		//Destination Address is the multisig addy 
		script outputScript = script(script().to_pay_script_hash_pattern(multisigAddy.hash()));

		//Amount to fill the channel with.

		output output1(bondAmount, outputScript);

		///Get UTXOs
		points_value UTXOs = getOutputs(payerAddy, bondAmount);
		//script previousLockingScript = script(script().to_pay_key_hash_pattern(payerAddy.hash()));

		//makes Inputs
		input::list inputs {};
		
		for (auto utxo: UTXOs.points)
		{
			input workingInput = input();
			workingInput.set_previous_output(output_point(utxo));

			std::cout << encode_base16(utxo.hash()) << std::endl;

			workingInput.set_sequence(0xffffffff);
			inputs.push_back(workingInput);
		}

		//Outputs List
		output::list outputs {output1};

		//Make Change
		if(UTXOs.value() > bondAmount)
		{
			outputs.push_back(makeChange(UTXOs.value()));
		}

		//Make Transaction 

		transaction tx = transaction();
		tx.set_inputs(inputs);
		tx.set_outputs(outputs);
		return tx;


	}
	void signBond()
	{
		endorsement sig;
		script previousLockingScript = script(script().to_pay_key_hash_pattern(payerAddy.hash()));

		script().create_endorsement(sig, wallet.childPrivateKey(HDindex).secret(), previousLockingScript, bond, 0, all);
		data_chunk rawKey = to_chunk(wallet.childPublicKey(HDindex).point());
		operation::list ops {operation(sig), operation(payerKey)};

		bond.inputs()[0].set_script(script(ops));

	}
	void signRefund()
	{
		endorsement sig;
		script().create_endorsement(sig, wallet.childPrivateKey(HDindex).secret(), redeemScript, refund, 0, all);
		operation::list ops {operation(sig)};
		refund.inputs()[0].set_script(script(ops));
	}

	client::connection_type connectionSettings()
	{
		client::connection_type connection = {};
		connection.retries = 3;
		connection.timeout_seconds = 8;
		connection.server = config::endpoint("tcp://testnet1.libbitcoin.net:19091");
		return connection;
	}


	// client::obelisk_client setClient()
	// {
	// 	client::connection_type connection = connectionSettings();
	// 	return client::obelisk_client client(connection);
	// }
	void validRefund(transaction refundTX)
		{
			endorsement endorse = refundTX.inputs()[0].script().to_data(1);
			ec_signature sig;

			parse_signature(sig, { endorse.begin(), endorse.end() - 1 },
	            0);
			if(script::check_signature(sig, endorse.back(), payerKey, redeemScript, refundTX, 0))
			{
				std::cout << "Confirmed" << std::endl;
			}else{
				std::cout << "Invalid" << std::endl;
			}
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

	transaction getBond()
	{
		return bond;
	}

	void broadcastTX(transaction tx)
	{
		client::obelisk_client client(connection);

		if(!client.connect(connection))
		{
			std::cout << "Fail" << std::endl;
		} else {
			std::cout << "Connection Succeeded" << std::endl;
		}
		
		static const auto on_done = [](const code& ec) {

			std::cout << "Success: " << ec.message() << std::endl;

		};

		static const auto on_error2 = [](const code& ec) {

			std::cout << "Error Code: " << ec.message() << std::endl;

		};

		client.transaction_pool_broadcast(on_error2, on_done, tx);
		client.wait();
	}

	transaction makeRefund()
	{
		script outputScript = script(script().to_pay_key_hash_pattern(payerAddy.hash()));

		//Amount in the channel to refund

		output output1((bondAmount-2000), outputScript);

		//Get UTXOs
		//points_value UTXOs = getOutputs(payerAddy, Satoshis);
		// hash_digest bondHash;
		// decode_hash(bondHash, "aad2ef50d74bd70892c023e81e531f061b0addbe0e4b5c5dc4354bb2c709c038");
		output_point utxo(bond.hash(), 0u);

		//makes Inputs
		input input1 = input();
		input1.set_previous_output(utxo);
		input1.set_sequence(0xffffffff);
		input::list inputs {input1};

		//Outputs List
		output::list outputs {output1};

		transaction tx = transaction();
		tx.set_inputs(inputs);
		tx.set_outputs(outputs);
		uint32_t now = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
		now = now + 86400;
		tx.set_locktime(now);
		return tx;

	}
	transaction getRefund()
	{
		return refund;
	}

private:
	data_chunk recieverKey;
	data_chunk payerKey;

	ec_public recieverPublic;
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





};