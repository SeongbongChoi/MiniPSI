#include "MiniSender.h"

#include <cryptoTools/Crypto/Commit.h>
#include <cryptoTools/Network/Channel.h>
#include <cryptoTools/Common/Timer.h>
#include <unordered_map>

namespace osuCrypto
{
    using namespace std;
	using namespace NTL;


	void MiniSender::init(u64 myInputSize, u64 theirInputSize, u64 psiSecParam, PRNG & prng, span<Channel> chls)
	{
		mPsiSecParam = psiSecParam;
		mMyInputSize = myInputSize;
		mTheirInputSize = theirInputSize;

		mPrng.SetSeed(prng.get<block>());
		mCurveSeed = mPrng.get<block>();
	
		simple.init(mTheirInputSize, recvMaxBinSize, recvNumDummies);

		EllipticCurve mCurve(p256k1, OneBlock);
		mFieldSize = mCurve.bitCount();

		std::cout << "s mFieldSize= " << mFieldSize << "\n";


		EccNumber nK(mCurve);
		EccPoint pG(mCurve);
		nK.randomize(mPrng);
		pG = mCurve.getGenerator();

		auto g_k = pG*nK;

		mG_K = new u8[g_k.sizeBytes()];
		mK = new u8[nK.sizeBytes()];

		g_k.toBytes(mG_K); //g^k
		nK.toBytes(mK);  //k

		EccPoint pGtest(mCurve);
		EccNumber nKTest(mCurve);
		pGtest.fromBytes(mG_K);
		nKTest.fromBytes(mK);
		std::cout << "s g^k= " << g_k << std::endl;
		std::cout << "s g^k= " << pGtest << std::endl;
		std::cout << "s k= " << nK << std::endl;
		std::cout << "s k= " << nKTest << std::endl;
	}

#if 0

	void PrtySender::output(span<block> inputs, span<Channel> chls)
	{
#if 1
		u64 numThreads(chls.size());
		const bool isMultiThreaded = numThreads > 1;
		std::mutex mtx;
		u64 polyMaskBytes = (mFieldSize + 7) / 8;
		u64 hashMaskBytes = (40+log2(mTheirInputSize*mMyInputSize)+7) / 8;
		u64 lastPolyMaskBytes = polyMaskBytes - (numSuperBlocks - 1) * sizeof(block);

		auto choiceBlocks = mOtChoices.getSpan<block>(); //s

		std::array<std::vector<u8>,2> globalHash;
		globalHash[0].resize(inputs.size()*hashMaskBytes);
		globalHash[1].resize(inputs.size()*hashMaskBytes);

		std::array<std::vector<u64>, 2>permute;
		int idxPermuteDone[2];
		for (u64 j = 0; j < 2; j++)
		{
			permute[j].resize(inputs.size());
			for (u64 i = 0; i < inputs.size(); i++)
				permute[j][i] = i;

			//permute position
			//std::shuffle(permute[j].begin(), permute[j].end(), mPrng);
			idxPermuteDone[j] = 0; //count the number of permutation that is done.
		}


		//=====================Balaced Allocation=====================
		//gTimer.reset();
		//gTimer.setTimePoint("start");
		simple.insertItems(inputs);
		gTimer.setTimePoint("s_binning");
		//std::cout << gTimer << std::endl;

		/*std::cout << IoStream::lock;
		simple.print(inputs);
		std::cout << IoStream::unlock;*/

		//=====================Compute OT row=====================
		auto routine = [&](u64 t)
		{
			auto& chl = chls[t];
			u64 binStartIdx = simple.mNumBins * t / numThreads;
			u64 tempBinEndIdx = (simple.mNumBins * (t + 1) / numThreads);
			u64 binEndIdx = std::min(tempBinEndIdx, simple.mNumBins);
			polyNTL poly;


#ifdef GF2X_Slicing
			poly.NtlPolyInit(sizeof(block));
			/*polyNTL poly_lastBlk;
			u64 lastBlkByteSize = polyMaskBytes - (numSuperBlocks - 1) * sizeof(block);
			poly_lastBlk.NtlPolyInit(lastBlkByteSize);*/
#else
			poly.NtlPolyInit(polyMaskBytes);
#endif // GF2X_Slicing

			


			for (u64 i = binStartIdx; i < binEndIdx; i += stepSize)
			{
				auto curStepSize = std::min(stepSize, binEndIdx - i);
				std::vector<std::vector<std::array<block, numSuperBlocks>>> rowQ(curStepSize);

				u64 iterSend = 0, iterRecv = 0;

				for (u64 k = 0; k < curStepSize; ++k)
				{
					u64 bIdx = i + k;
					rowQ[k].resize(simple.mBins[bIdx].blks.size());
					//=====================Compute OT row=====================
					prfOtRows(simple.mBins[bIdx].blks, rowQ[k], mAesQ);

				}

				std::vector<u8> recvBuff;
				chl.recv(recvBuff); //receive Poly
		/*			if (recvBuff.size() != curStepSize*simple.mTheirMaxBinSize*polyMaskBytes);
					{
						int aa = curStepSize*simple.mTheirMaxBinSize*polyMaskBytes;
						std::cout << recvBuff.size() << "\t" <<aa << std::endl;

						std::cout << "error @ " << (LOCATION) << std::endl;
						throw std::runtime_error(LOCATION);
					}*/

					//=====================Unpack=====================
				for (u64 k = 0; k < curStepSize; ++k)
				{
					u64 bIdx = i + k;
					u64 realNumRows = simple.mBins[bIdx].blks.size();
					
#ifdef GF2X_Slicing
					std::vector<block> localHashes(realNumRows);
					u64 degree = simple.mTheirMaxBinSize - 1;
					std::vector<block> X(realNumRows), R(realNumRows), coeffs(degree+1); //
					block rcvBlk;
					NTL::GF2E e;
					NTL::vec_GF2E vecX;

					for (u64 idx = 0; idx < realNumRows; ++idx)
					{
						poly.GF2EFromBlock(e, inputs[simple.mBins[bIdx].values[idx].mIdx], poly.mNumBytes);
						vecX.append(e);
					}

					for (u64 j = 0; j < numSuperBlocks; ++j) //slicing
					{
						//if (j == numSuperBlocks - 1)
						//{
						//	for (int c = 0; c < coeffs.size(); c++) {
						//		memcpy((u8*)&coeffs[c], recvBuff.data() + iterSend, lastBlkByteSize);
						//		iterSend += lastBlkByteSize;
						//	}
						//	poly_lastBlk.evalPolynomial(coeffs, vecX, R);
						//}
						//else
						{
							for (int c = 0; c < coeffs.size(); c++) {
								memcpy((u8*)&coeffs[c], recvBuff.data() + iterSend, sizeof(block));
								iterSend += sizeof(block);
							}
							poly.evalPolynomial(coeffs, vecX, R);
						}

						for (int idx = 0; idx < realNumRows; idx++) {

							rcvBlk = rowQ[iterRowQ+idx][j] ^ (R[idx] & choiceBlocks[j]); //Q+s*P

							/*if (j == numSuperBlocks - 1)
								rcvBlk = rcvBlk&mTruncateBlk;*/

							if (bIdx == 2 && idx == 0)
								std::cout << "R[idx]" << R[idx] << "\t" << rcvBlk<<"\t"<<"\n";
							
							localHashes[idx] = simple.mAesHasher.ecbEncBlock(rcvBlk) ^ localHashes[idx]; //compute H(Q+s*P)=xor of all slices
						}

					}

#else
					u64 degree = simple.mTheirMaxBinSize - 1;
					std::vector<std::array<block, numSuperBlocks>> R(realNumRows), coeffs(degree + 1); //
					block rcvBlk;

					
					for (int c = 0; c < coeffs.size(); c++)
					{
						memcpy((u8*)&coeffs[c], recvBuff.data() + iterSend, polyMaskBytes);
						iterSend += polyMaskBytes;
					}

					poly.evalSuperPolynomial(coeffs, simple.mBins[bIdx].blks, R);

					//if (bIdx == 2)
					//{
					//	std::cout << "rX= " << X[0] << "\t X2.size() " << X.size() << "\t  coeffs.size()" << coeffs.size() << "\n";
					//	std::cout << "rY= ";

					//	for (int j = 0; j < numSuperBlocks; ++j) {
					//		std::cout << R[0][j] << "\t";
					//	}
					//	std::cout << "\n";
					//}


					std::array<block, numSuperBlocks> recvRowT;
					std::vector<block> cipher(4);

					for (int idx = 0; idx < realNumRows; idx++) {
						for (u64 j = 0; j < numSuperBlocks; ++j) //slicing
						{
							recvRowT[j] = rowQ[k][idx][j] ^ (R[idx][j] & choiceBlocks[j]); //Q+s*P

							if (j == numSuperBlocks - 1) //get last 440-3*128 bits
								recvRowT[j] = recvRowT[j] & mTruncateBlk;
						}

						simple.mAesHasher.ecbEncFourBlocks(recvRowT.data(), cipher.data());

						for (u64 j = 1; j < numSuperBlocks; ++j)
							cipher[0] = cipher[0] ^ cipher[j];
						
						u64 hashIdx = simple.mBins[bIdx].hashIdxs[idx];
						memcpy(globalHash[hashIdx].data() + permute[hashIdx][idxPermuteDone[hashIdx]++] * hashMaskBytes
							, (u8*)&cipher[0], hashMaskBytes);

					}
					

#endif
					/*if (bIdx == bIdxForDebug)
					{
						idxPermuteDoneforDebug = idxPermuteDone[1];

						std::cout << "sendMask " << localHashes[iIdxForDebug]
							<< " X= " << inputs[simple.mBins[bIdx].values[iIdxForDebug].mIdx]
							<< " hIdx " << simple.mBins[bIdx].values[iIdxForDebug].mHashIdx 
							<<" idxPer " << idxPermuteDoneforDebug << "\n";
					}*/

				}

	}
		};

		

	
		std::vector<std::thread> thrds(chls.size());
		for (u64 i = 0; i < thrds.size(); ++i)
		{
			thrds[i] = std::thread([=] {
				routine(i);
			});
		}

		for (auto& thrd : thrds)
			thrd.join();

		gTimer.setTimePoint("s_poly");

#ifdef PSI_PRINT
	
		
		for (u64 hIdx = 0; hIdx < 2; hIdx++)
			for (u64 k = 0; k < inputs.size(); ++k)
			{
				block globalTest;
				memcpy((u8*)&globalTest, globalHash[hIdx].data() + k* hashMaskBytes, hashMaskBytes);
				std::cout << IoStream::lock;
				std::cout << "globalHash " << hIdx << " " << k << "\t" << globalTest << "\n";
				std::cout << IoStream::unlock;
			}

		block globalTest;
		memcpy((u8*)&globalTest, globalHash[1].data() + permute[1][idxPermuteDoneforDebug] * hashMaskBytes, hashMaskBytes);
		std::cout << "globalHash " << globalTest << "\n";

#endif // PSI_PRINT


		auto sendingMask = [&](u64 t)
		{
			auto& chl = chls[t]; //parallel along with inputs
			u64 startIdx = inputs.size() * t / numThreads;
			u64 tempEndIdx = (inputs.size() * (t + 1) / numThreads);
			u64 endIdx = std::min(tempEndIdx, (u64)inputs.size());


			for (u64 i = startIdx; i < endIdx; i += stepSizeMaskSent)
			{
				auto curStepSize = std::min(stepSizeMaskSent, endIdx - i);

				for (u64 hIdx = 0; hIdx < 2; hIdx++)
				{
					std::vector<u8> sendBuff(curStepSize*hashMaskBytes);
					memcpy(sendBuff.data(), globalHash[hIdx].data() + i*hashMaskBytes, curStepSize*hashMaskBytes);
					chl.asyncSend(std::move(sendBuff));

#ifdef PSI_PRINT
					for (u64 k = 0; k < curStepSize; ++k)
					{
						block globalTest;
						memcpy((u8*)&globalTest, sendBuff[hIdx].data() + k* hashMaskBytes, hashMaskBytes);
						std::cout << IoStream::lock;
						std::cout << "sendBuffs " << hIdx << " " << k << "\t" << globalTest << "\n";
						std::cout << IoStream::unlock;
					}
#endif // PSI_PRINT
				}
				
			}
		};

		for (u64 i = 0; i < thrds.size(); ++i)//thrds.size()
		{
			thrds[i] = std::thread([=] {
				sendingMask(i);
			});
		}

		for (auto& thrd : thrds)
			thrd.join();

		
#endif
	}

	//static bool wayToSort(block i,  block j) { 
	//	if (i < j)
	//		return true;

	//	return false;

	//	//return static_cast<bool>(i < j); 
	//
	//}

	void PrtySender::outputBestComm(span<block> inputs, span<Channel> chls)
	{
#if 1
		u64 numThreads(chls.size());
		const bool isMultiThreaded = numThreads > 1;
		std::mutex mtx;
		u64 polyMaskBytes = (mFieldSize + 7) / 8;
		
		u64 hashMaskBits = (40 + log2(mTheirInputSize) + 2);
		u64 hashMaskBytes = (hashMaskBits + 7) / 8;

		u64 n1n2MaskBits = (40 + log2(mTheirInputSize*mMyInputSize));
		u64 n1n2MaskBytes = (n1n2MaskBits + 7) / 8;


		auto choiceBlocks = mOtChoices.getSpan<block>(); //s

		std::array<std::vector<block>, 2> globalHash;
		globalHash[0].resize(inputs.size());
		globalHash[1].resize(inputs.size());

		std::array<std::vector<u64>, 2>permute;
		int idxPermuteDone[2];
		for (u64 j = 0; j < 2; j++)
		{
			permute[j].resize(inputs.size());
			for (u64 i = 0; i < inputs.size(); i++)
				permute[j][i] = i;

			//permute position
			//std::shuffle(permute[j].begin(), permute[j].end(), mPrng);
			idxPermuteDone[j] = 0; //count the number of permutation that is done.
		}


		//=====================Balaced Allocation=====================
		//gTimer.reset();
		//gTimer.setTimePoint("start");
		simple.insertItems(inputs);
		gTimer.setTimePoint("s_binning");
		//std::cout << gTimer << std::endl;

		/*std::cout << IoStream::lock;
		simple.print(inputs);
		std::cout << IoStream::unlock;*/

		//=====================Compute OT row=====================
		auto routine = [&](u64 t)
		{
			auto& chl = chls[t];
			u64 binStartIdx = simple.mNumBins * t / numThreads;
			u64 tempBinEndIdx = (simple.mNumBins * (t + 1) / numThreads);
			u64 binEndIdx = std::min(tempBinEndIdx, simple.mNumBins);
			polyNTL poly;


#ifdef GF2X_Slicing
			poly.NtlPolyInit(sizeof(block));
			/*polyNTL poly_lastBlk;
			u64 lastBlkByteSize = polyMaskBytes - (numSuperBlocks - 1) * sizeof(block);
			poly_lastBlk.NtlPolyInit(lastBlkByteSize);*/
#else
			poly.NtlPolyInit(polyMaskBytes);
#endif // GF2X_Slicing




			for (u64 i = binStartIdx; i < binEndIdx; i += stepSize)
			{
				auto curStepSize = std::min(stepSize, binEndIdx - i);
				std::vector<std::vector<std::array<block, numSuperBlocks>>> rowQ(curStepSize);

				u64 iterSend = 0, iterRecv = 0;

				for (u64 k = 0; k < curStepSize; ++k)
				{
					u64 bIdx = i + k;
					rowQ[k].resize(simple.mBins[bIdx].blks.size());
					//=====================Compute OT row=====================
					prfOtRows(simple.mBins[bIdx].blks, rowQ[k], mAesQ);

				}

				std::vector<u8> recvBuff;
				chl.recv(recvBuff); //receive Poly
									/*			if (recvBuff.size() != curStepSize*simple.mTheirMaxBinSize*polyMaskBytes);
									{
									int aa = curStepSize*simple.mTheirMaxBinSize*polyMaskBytes;
									std::cout << recvBuff.size() << "\t" <<aa << std::endl;

									std::cout << "error @ " << (LOCATION) << std::endl;
									throw std::runtime_error(LOCATION);
									}*/

									//=====================Unpack=====================
				for (u64 k = 0; k < curStepSize; ++k)
				{
					u64 bIdx = i + k;
					u64 realNumRows = simple.mBins[bIdx].blks.size();

#ifdef GF2X_Slicing
					std::vector<block> localHashes(realNumRows);
					u64 degree = simple.mTheirMaxBinSize - 1;
					std::vector<block> X(realNumRows), R(realNumRows), coeffs(degree + 1); //
					block rcvBlk;
					NTL::GF2E e;
					NTL::vec_GF2E vecX;

					for (u64 idx = 0; idx < realNumRows; ++idx)
					{
						poly.GF2EFromBlock(e, inputs[simple.mBins[bIdx].values[idx].mIdx], poly.mNumBytes);
						vecX.append(e);
					}

					for (u64 j = 0; j < numSuperBlocks; ++j) //slicing
					{
						//if (j == numSuperBlocks - 1)
						//{
						//	for (int c = 0; c < coeffs.size(); c++) {
						//		memcpy((u8*)&coeffs[c], recvBuff.data() + iterSend, lastBlkByteSize);
						//		iterSend += lastBlkByteSize;
						//	}
						//	poly_lastBlk.evalPolynomial(coeffs, vecX, R);
						//}
						//else
						{
							for (int c = 0; c < coeffs.size(); c++) {
								memcpy((u8*)&coeffs[c], recvBuff.data() + iterSend, sizeof(block));
								iterSend += sizeof(block);
							}
							poly.evalPolynomial(coeffs, vecX, R);
						}

						for (int idx = 0; idx < realNumRows; idx++) {

							rcvBlk = rowQ[iterRowQ + idx][j] ^ (R[idx] & choiceBlocks[j]); //Q+s*P

																						   /*if (j == numSuperBlocks - 1)
																						   rcvBlk = rcvBlk&mTruncateBlk;*/

							if (bIdx == 2 && idx == 0)
								std::cout << "R[idx]" << R[idx] << "\t" << rcvBlk << "\t" << "\n";

							localHashes[idx] = simple.mAesHasher.ecbEncBlock(rcvBlk) ^ localHashes[idx]; //compute H(Q+s*P)=xor of all slices
						}

					}

#else
					u64 degree = simple.mTheirMaxBinSize - 1;
					std::vector<std::array<block, numSuperBlocks>> R(realNumRows), coeffs(degree + 1); //
					block rcvBlk;


					for (int c = 0; c < coeffs.size(); c++)
					{
						memcpy((u8*)&coeffs[c], recvBuff.data() + iterSend, polyMaskBytes);
						iterSend += polyMaskBytes;
					}

					poly.evalSuperPolynomial(coeffs, simple.mBins[bIdx].blks, R);

					//if (bIdx == 2)
					//{
					//	std::cout << "rX= " << X[0] << "\t X2.size() " << X.size() << "\t  coeffs.size()" << coeffs.size() << "\n";
					//	std::cout << "rY= ";

					//	for (int j = 0; j < numSuperBlocks; ++j) {
					//		std::cout << R[0][j] << "\t";
					//	}
					//	std::cout << "\n";
					//}


					std::array<block, numSuperBlocks> recvRowT;
					std::vector<block> cipher(4);

					for (int idx = 0; idx < realNumRows; idx++) {
						for (u64 j = 0; j < numSuperBlocks; ++j) //slicing
						{
							recvRowT[j] = rowQ[k][idx][j] ^ (R[idx][j] & choiceBlocks[j]); //Q+s*P

							if (j == numSuperBlocks - 1) //get last 440-3*128 bits
								recvRowT[j] = recvRowT[j] & mTruncateBlk;
						}

						simple.mAesHasher.ecbEncFourBlocks(recvRowT.data(), cipher.data());

						for (u64 j = 1; j < numSuperBlocks; ++j)
							cipher[0] = cipher[0] ^ cipher[j];

						u64 hashIdx = simple.mBins[bIdx].hashIdxs[idx];
						
						//memcpy((u8*)&globalHash[hashIdx][simple.mBins[bIdx].Idxs[idx]], (u8*)&cipher[0], sizeof(block));

						//globalHash[hashIdx][idxPermuteDone[hashIdx]++] = cipher[0];
						if (isMultiThreaded)
						{
							std::lock_guard<std::mutex> lock(mtx);
							memcpy(globalHash[hashIdx].data() + permute[hashIdx][idxPermuteDone[hashIdx]++]
								, (u8*)&cipher[0], sizeof(block));
						}
						else
							memcpy(globalHash[hashIdx].data() + permute[hashIdx][idxPermuteDone[hashIdx]++]
								, (u8*)&cipher[0], sizeof(block));

					}


#endif
					/*if (bIdx == bIdxForDebug)
					{
					idxPermuteDoneforDebug = idxPermuteDone[1];

					std::cout << "sendMask " << localHashes[iIdxForDebug]
					<< " X= " << inputs[simple.mBins[bIdx].values[iIdxForDebug].mIdx]
					<< " hIdx " << simple.mBins[bIdx].values[iIdxForDebug].mHashIdx
					<<" idxPer " << idxPermuteDoneforDebug << "\n";
					}*/

				}

			}
		};




		std::vector<std::thread> thrds(chls.size());
		for (u64 i = 0; i < thrds.size(); ++i)
		{
			thrds[i] = std::thread([=] {
				routine(i);
			});
		}

		for (auto& thrd : thrds)
			thrd.join();

		gTimer.setTimePoint("s_poly");

#ifdef PSI_PRINT


		for (u64 hIdx = 0; hIdx < 2; hIdx++)
			for (u64 k = 0; k < inputs.size(); ++k)
			{
				block globalTest;
				memcpy((u8*)&globalTest, globalHash[hIdx].data() + k* hashMaskBytes, hashMaskBytes);
				std::cout << IoStream::lock;
				std::cout << "globalHash " << hIdx << " " << k << "\t" << globalTest << "\n";
				std::cout << IoStream::unlock;
			}

		block globalTest;
		memcpy((u8*)&globalTest, globalHash[1].data() + permute[1][idxPermuteDoneforDebug] * hashMaskBytes, hashMaskBytes);
		std::cout << "globalHash " << globalTest << "\n";

#endif // PSI_PRINT


		//=====================Sort & sending masks=====================
		

		auto compareBlockFunction = [](const block& lhs, const block& rhs) -> bool {
			return memcmp(&lhs, &rhs, sizeof(block)) < 0;
		};

		
			auto sendingMasks = [&](u64 hIdx, Channel chl)
			{
				

				//std::cout << globalHash[hIdx].size() << " globalHash.size()\n";

				std::sort(globalHash[hIdx].begin(), globalHash[hIdx].end(), compareBlockFunction);

				gTimer.setTimePoint("s_sort");

				std::vector<u8> sendBuff(1.02*inputs.size()*(hashMaskBytes));


				//block 
				block boundMaskDiff = ZeroBlock;
				for (u64 i = 0; i < hashMaskBytes * 8; i++)
					boundMaskDiff = boundMaskDiff^mOneBlocks[i];

				//std::cout << boundMaskDiff << "  boundMaskDiff\n";



				u64 iterSendDiff = 0;

				memcpy(sendBuff.data(), (u8*)&globalHash[hIdx][0], n1n2MaskBytes);
				iterSendDiff += n1n2MaskBytes;


				/*block aaa = ZeroBlock;
				memcpy((u8*)&aaa, sendBuff.data(), n1n2MaskBytes);
				std::cout << aaa << " sendBuff[0] \t" << globalHash[hIdx][0] << "\n";*/


				block diff;
				for (u64 idx = 0; idx < inputs.size() - 1; idx++)
				{
					diff = globalHash[hIdx][idx + 1] - globalHash[hIdx][idx];

					if (memcmp(&diff, &boundMaskDiff, hashMaskBytes) < 0)
					{
						//std::cout << diff << "  " << idx << "\t ==diff==\t" << globalHash[idx + 1] << "\t" << globalHash[idx] << "\n";
						memcpy(sendBuff.data() + iterSendDiff, (u8*)&diff, hashMaskBytes);
						iterSendDiff += hashMaskBytes;
					}
					else
					{
						//std::cout << diff << "  " << idx << "\t ==dddddiff==\t" << globalHash[hIdx][idx + 1] << "\t" << globalHash[hIdx][idx] << "\n";

						memcpy(sendBuff.data() + iterSendDiff, (u8*)&ZeroBlock, hashMaskBytes);
						iterSendDiff += hashMaskBytes;

						memcpy(sendBuff.data() + iterSendDiff, (u8*)& globalHash[hIdx][idx + 1], n1n2MaskBytes);
						iterSendDiff += n1n2MaskBytes;
					}
					if (iterSendDiff > sendBuff.size())
					{
						std::cout << "iterSendDiff > sendBuff.size(): " << iterSendDiff << "\t" << sendBuff.size() << "\n";
						sendBuff.resize(sendBuff.size() + (inputs.size() - iterSendDiff)*hashMaskBytes);
					}

					/*std::cout << IoStream::lock;
					std::cout << "s mask: " << idx << "  " << globalHash[hIdx][idx + 1] << " - " << globalHash[hIdx][idx] << " ===diff:===" << diff << "\n";
					std::cout << IoStream::unlock;*/

				}
				//memcpy(sendBuff.data() + iterSendDiff, (u8*)& ZeroBlock, sendBuff.size()- iterSendDiff);




				chl.asyncSend(std::move(sendBuff));
			};

			if (isMultiThreaded)
			{
				for (u64 i = 0; i < 2; ++i)
				{
					thrds[i] = std::thread([=] {
						sendingMasks(i, chls[i]);
					});
				}

				for (u64 i = 0; i < 2; ++i)
					thrds[i].join();
			}
			else
			{
				sendingMasks(0, chls[0]);
				sendingMasks(1, chls[0]);

			}


#endif
	}

#endif
	void MiniSender::outputBigPoly(span<block> inputs, span<Channel> chls)
	{
		chls[0].asyncSend(mG_K);

		EllipticCurve mCurve(p256k1, OneBlock);

		EccNumber nK(mCurve);
		nK.fromBytes(mK);


		u64 numThreads(chls.size());
		const bool isMultiThreaded = numThreads > 1;
		std::vector<std::thread> thrds(numThreads);
		std::mutex mtx;

		u64 polyMaskBytes = 33;
		u64 hashMaskBits = (40 + log2(mTheirInputSize) + 2 ) ;
		u64 hashMaskBytes =  (hashMaskBits + 7) / 8;

		u64 n1n2MaskBits = (40 + log2(mTheirInputSize*mMyInputSize));
		u64 n1n2MaskBytes = (n1n2MaskBits+7)/8;


		//=====================Poly=====================
		u64 degree = mTheirInputSize - 1;
		
			mPrime = mPrime264;
			ZZ_p::init(ZZ(mPrime));

			ZZ_p* zzX = new ZZ_p[inputs.size()];
			ZZ_p* zzY = new ZZ_p[inputs.size()];
			ZZ zz;

			for (u64 idx = 0; idx < inputs.size(); idx++)
			{
				ZZFromBytes(zz, (u8*)&inputs[idx], sizeof(block));
				zzX[idx] = to_ZZ_p(zz);
			}

			ZZ_pX* p_tree = new ZZ_pX[degree * 2 + 1];
			ZZ_pX* reminders = new ZZ_pX[degree * 2 + 1];
			

			build_tree(p_tree, zzX, degree * 2 + 1, numThreads, mPrime);
			u8* rcvBlk=new u8[polyMaskBytes];

			ZZ_pX recvPolynomial;

			std::vector<u8> recvBuffs;

			
				u64 iterRecvs = 0;
				chls[0].recv(recvBuffs);

				std::cout << "s recvBuffs[idxBlk].size(): " << recvBuffs.size() << std::endl;

				for (int c = 0; c <= degree; c++) {
					memcpy((u8*)&rcvBlk, recvBuffs.data() + iterRecvs, polyMaskBytes);
					iterRecvs += polyMaskBytes;

					std::cout << "s SetCoeff rcvBlk= " <<c << " - " << rcvBlk << std::endl;

					ZZFromBytes(zz, (u8*)&rcvBlk, polyMaskBytes);
					SetCoeff(recvPolynomial, c, to_ZZ_p(zz));
				}
				evaluate(recvPolynomial, p_tree, reminders, degree * 2 + 1, zzY, numThreads, mPrime);

				for (u64 idx = 0; idx < inputs.size(); idx++)
				{
					block pY;
					BytesFromZZ((u8*)&pY, rep(zzY[idx]), polyMaskBytes);
					std::cout << "s P(y)= " << idx << " - " << pY << std::endl;

				}


		auto computeGlobalHash = [&](u64 t)
		{
			u64 startIdx = mMyInputSize * t / numThreads;
			u64 tempEndIdx = mMyInputSize* (t + 1) / numThreads;
			u64 endIdx = std::min(tempEndIdx, mMyInputSize);

			std::vector<u8> sendBuff(n1n2MaskBytes * (endIdx - startIdx));
			auto sendIter = sendBuff.data();


			std::cout << "s startIdx= " << startIdx << std::endl;
			std::cout << "s endIdx= " << endIdx << std::endl;

			EccPoint point_ri(mCurve);

			for (u64 idx = 0; idx < endIdx - startIdx; idx++)
			{
				std::cout << "s idx= " << idx << std::endl;
				u64 idxItem = startIdx + idx;

				
				u8* yri = new u8[point_ri.sizeBytes()];
				BytesFromZZ(yri , rep(zzY[idxItem]), sizeof(block));

				std::cout << "s yri= " << toBlock(yri) << std::endl;
				std::cout << "s yri= " << toBlock(yri+ sizeof(block)) << std::endl;

				point_ri.fromBytes(yri);

				std::cout << "s point_ri= " << point_ri << std::endl;

				/*auto yri_K = point_ri*nK;
				std::cout << "s yri_K= " << yri_K << std::endl;


				yri_K.toBytes(sendIter);
				sendIter += n1n2MaskBytes;*/
				
			}
		//	chls[t].asyncSend(std::move(sendBuff));	//send H(x)^a
		};


		for (u64 i = 0; i < thrds.size(); ++i)
		{
			thrds[i] = std::thread([=] {
				computeGlobalHash(i);
			});
		}
		for (auto& thrd : thrds)
			thrd.join();

		gTimer.setTimePoint("computeMask");

#if 0
		//=====================Sort=====================



		//std::cout << globalHash.size() << " globalHash.size()\n";

		auto compareBlockFunction = [](const block& lhs, const block& rhs) -> bool {
			return memcmp(&lhs, &rhs, sizeof(block)) < 0;
		};

		std::sort(globalHash.begin(), globalHash.end(), compareBlockFunction);
		gTimer.setTimePoint("s_sort");

		//block 
		block boundMaskDiff = ZeroBlock;
		for (u64 i = 0; i < hashMaskBytes * 8; i++)
			boundMaskDiff = boundMaskDiff^mOneBlocks[i];
		//std::cout << boundMaskDiff << "  boundMaskDiff\n";

		auto sendingMask = [&](u64 t)
		{
			auto& chl = chls[t];

			u64 startIdx = mMyInputSize * t / numThreads;
			u64 tempEndIdx = mMyInputSize* (t + 1) / numThreads;
			u64 endIdx = std::min(tempEndIdx, mMyInputSize);

			
			/*block aaa = ZeroBlock;
			memcpy((u8*)&aaa, sendBuff.data(), n1n2MaskBytes);
			std::cout << aaa << " sendBuff[0] \t" << globalHash[0] << "\n";*/


			block diff;
			for (u64 i = startIdx; i < endIdx - 1; i += stepSizeMaskSent)
			{
				auto curStepSize = std::min(stepSizeMaskSent, endIdx-1 - i);


				std::vector<u8> sendBuff(1.02*curStepSize*(hashMaskBytes));

				u64 iterSendDiff = 0;

				memcpy(sendBuff.data(), (u8*)&globalHash[i], n1n2MaskBytes);
				iterSendDiff += n1n2MaskBytes;

				for (u64 k = 0; k < curStepSize; ++k)
				{
					u64 idx = i + k;

					diff = globalHash[idx + 1] - globalHash[idx];

					if (memcmp(&diff, &boundMaskDiff, hashMaskBytes) < 0)
					{
						//std::cout << diff << "  " << idx << "\t ==diff==\t" << globalHash[idx + 1] << "\t" << globalHash[idx] << "\n";
						memcpy(sendBuff.data() + iterSendDiff, (u8*)&diff, hashMaskBytes);
						iterSendDiff += hashMaskBytes;
					}
					else
					{
						//std::cout << diff << "  " << idx << "\t ==dddddiff==\t" << globalHash[idx + 1] << "\t" << globalHash[idx] << "\n";

						memcpy(sendBuff.data() + iterSendDiff, (u8*)&ZeroBlock, hashMaskBytes);
						iterSendDiff += hashMaskBytes;

						memcpy(sendBuff.data() + iterSendDiff, (u8*)& globalHash[idx + 1], n1n2MaskBytes);
						iterSendDiff += n1n2MaskBytes;
					}
					if (iterSendDiff > sendBuff.size())
					{
						std::cout << "iterSendDiff > sendBuff.size(): " << iterSendDiff << "\t" << sendBuff.size() << "\n";
						sendBuff.resize(sendBuff.size() + (inputs.size() - iterSendDiff)*hashMaskBytes);
					}

					//std::cout << "s mask: " << idx << "  " << globalHash[idx+1] << " - " <<globalHash[idx] << " ===diff:===" << diff << "\n";

				}
				//memcpy(sendBuff.data() + iterSendDiff, (u8*)& ZeroBlock, sendBuff.size()- iterSendDiff);


				chl.asyncSend(std::move(sendBuff));
			}
		};

	
		for (u64 i = 0; i < thrds.size(); ++i)//thrds.size()
		{
			thrds[i] = std::thread([=] {
				sendingMask(i);
			});
		}

		for (auto& thrd : thrds)
			thrd.join();


#endif
	}


}

