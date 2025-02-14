/*
    This file is part of SX1280 Portable driver.
    Copyright (C) 2020 ReimuNotMoe

    This program is based on sx1280-driver from Semtech S.A.,
    see LICENSE-SEMTECH.txt for details.

    Original maintainer of sx1280-driver: Miguel Luis, Gregory Cristian
    and Matthieu Verdy

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as
    published by the Free Software Foundation, either version 3 of the
    License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include "SX128x.hpp"

void SX128x::Init() {
	Reset();
	Wakeup();
//	SetRegistersDefault();
}

//void SX128x::SetRegistersDefault(void )
//{
//	for( int16_t i = 0; i < sizeof( RadioRegsInit ) / sizeof( RadioRegisters_t ); i++ ) {
//		WriteRegister( RadioRegsInit[i].Addr, RadioRegsInit[i].Value );
//	}
//}

uint16_t SX128x::GetFirmwareVersion(void )
{
	return( ( ( ReadRegister( REG_LR_FIRMWARE_VERSION_MSB ) ) << 8 ) | ( ReadRegister( REG_LR_FIRMWARE_VERSION_MSB + 1 ) ) );
}

SX128x::RadioStatus_t SX128x::GetStatus(void )
{
	uint8_t stat = 0;
	RadioStatus_t status;

	ReadCommand( RADIO_GET_STATUS, ( uint8_t * )&stat, 1 );
	status.Value = stat;
	return( status );
}

SX128x::RadioOperatingModes_t SX128x::GetOpMode(void )
{
	return( OperatingMode );
}

void SX128x::SetSleep(SleepParams_t sleepConfig )
{
	uint8_t sleep = ( sleepConfig.WakeUpRTC << 3 ) |
			( sleepConfig.InstructionRamRetention << 2 ) |
			( sleepConfig.DataBufferRetention << 1 ) |
			( sleepConfig.DataRamRetention );

	OperatingMode = MODE_SLEEP;
	WriteCommand( RADIO_SET_SLEEP, &sleep, 1 );
}

void SX128x::SetStandby(RadioStandbyModes_t standbyConfig )
{
	// std::lock_guard<std::mutex> lg(IOLock2);

	WriteCommand( RADIO_SET_STANDBY, ( uint8_t* )&standbyConfig, 1 );
	if (standbyConfig == STDBY_RC )
	{
		OperatingMode = MODE_STDBY_RC;
	}
	else
	{
		OperatingMode = MODE_STDBY_XOSC;
	}
}

void SX128x::SetFs(void )
{
	WriteCommand( RADIO_SET_FS, 0, 0 );
	OperatingMode = MODE_FS;
}

void SX128x::SetTx(TickTime_t timeout )
{
	// std::lock_guard<std::mutex> lg(IOLock2);

	uint8_t buf[3];
	buf[0] = timeout.PeriodBase;
	buf[1] = ( uint8_t )( ( timeout.PeriodBaseCount >> 8 ) & 0x00FF );
	buf[2] = ( uint8_t )( timeout.PeriodBaseCount & 0x00FF );

	ClearIrqStatus( IRQ_RADIO_ALL );

	// If the radio is doing ranging operations, then apply the specific calls
	// prior to SetTx
//	if (GetPacketType( true ) == PACKET_TYPE_RANGING )
//	{
////		SetRangingRole( RADIO_RANGING_ROLE_MASTER );
//	}

	HalPostRx();
	HalPreTx();
	WriteCommand( RADIO_SET_TX, buf, 3 );
	OperatingMode = MODE_TX;
}

void SX128x::SetRx(TickTime_t timeout )
{
	// std::lock_guard<std::mutex> lg(IOLock2);

	uint8_t buf[3];
	buf[0] = timeout.PeriodBase;
	buf[1] = ( uint8_t )( ( timeout.PeriodBaseCount >> 8 ) & 0x00FF );
	buf[2] = ( uint8_t )( timeout.PeriodBaseCount & 0x00FF );

//	ClearIrqStatus( IRQ_RADIO_ALL );

	// If the radio is doing ranging operations, then apply the specific calls
	// prior to SetRx
//	if (GetPacketType( true ) == PACKET_TYPE_RANGING )
//	{
////		SetRangingRole( RADIO_RANGING_ROLE_SLAVE );
//	}

	HalPostTx();
	HalPreRx();
	WriteCommand( RADIO_SET_RX, buf, 3 );
	OperatingMode = MODE_RX;
}

void SX128x::SetRxDutyCycle(RadioTickSizes_t periodBase, uint16_t periodBaseCountRx, uint16_t periodBaseCountSleep )
{
	uint8_t buf[5];

	buf[0] = periodBase;
	buf[1] = ( uint8_t )( ( periodBaseCountRx >> 8 ) & 0x00FF );
	buf[2] = ( uint8_t )( periodBaseCountRx & 0x00FF );
	buf[3] = ( uint8_t )( ( periodBaseCountSleep >> 8 ) & 0x00FF );
	buf[4] = ( uint8_t )( periodBaseCountSleep & 0x00FF );

	HalPostTx();
	HalPreRx();
	WriteCommand( RADIO_SET_RXDUTYCYCLE, buf, 5 );
	OperatingMode = MODE_RX;
}

void SX128x::SetCad(void )
{
	// std::lock_guard<std::mutex> lg(IOLock2);

	HalPostTx();
	HalPreRx();
	WriteCommand( RADIO_SET_CAD, 0, 0 );
	OperatingMode = MODE_CAD;
}

void SX128x::SetTxContinuousWave(void )
{
	// std::lock_guard<std::mutex> lg(IOLock2);

	HalPostRx();
	HalPreTx();
	WriteCommand( RADIO_SET_TXCONTINUOUSWAVE, 0, 0 );
}

void SX128x::SetTxContinuousPreamble(void )
{
	// std::lock_guard<std::mutex> lg(IOLock2);

	HalPostRx();
	HalPreTx();
	WriteCommand( RADIO_SET_TXCONTINUOUSPREAMBLE, 0, 0 );
}

void SX128x::SetPacketType(RadioPacketTypes_t packetType )
{
	// Save packet type internally to avoid questioning the radio
	this->PacketType = packetType;

	WriteCommand( RADIO_SET_PACKETTYPE, ( uint8_t* )&packetType, 1 );
}

SX128x::RadioPacketTypes_t SX128x::GetPacketType(bool returnLocalCopy )
{
	RadioPacketTypes_t packetType = PACKET_TYPE_NONE;
	if (returnLocalCopy == false )
	{
		ReadCommand( RADIO_GET_PACKETTYPE, ( uint8_t* )&packetType, 1 );
		if (this->PacketType != packetType )
		{
			this->PacketType = packetType;
		}
	}
	else
	{
		packetType = this->PacketType;
	}
	return packetType;
}

void SX128x::SetRfFrequency(uint32_t rfFrequency )
{
	uint8_t buf[3];
	uint32_t freq = 0;

	freq = ( uint32_t )( ( double )rfFrequency / ( double )FREQ_STEP );
	buf[0] = ( uint8_t )( ( freq >> 16 ) & 0xFF );
	buf[1] = ( uint8_t )( ( freq >> 8 ) & 0xFF );
	buf[2] = ( uint8_t )( freq & 0xFF );
	WriteCommand( RADIO_SET_RFFREQUENCY, buf, 3 );
}

void SX128x::SetTxParams(int8_t power, RadioRampTimes_t rampTime )
{
	uint8_t buf[2];

	// The power value to send on SPI/UART is in the range [0..31] and the
	// physical output power is in the range [-18..13]dBm
	buf[0] = power + 18;
	buf[1] = ( uint8_t )rampTime;
	WriteCommand( RADIO_SET_TXPARAMS, buf, 2 );
}

void SX128x::SetCadParams(RadioLoRaCadSymbols_t cadSymbolNum )
{
	WriteCommand( RADIO_SET_CADPARAMS, ( uint8_t* )&cadSymbolNum, 1 );
	OperatingMode = MODE_CAD;
}

void SX128x::SetBufferBaseAddresses(uint8_t txBaseAddress, uint8_t rxBaseAddress )
{
	uint8_t buf[2];

	buf[0] = txBaseAddress;
	buf[1] = rxBaseAddress;
	WriteCommand( RADIO_SET_BUFFERBASEADDRESS, buf, 2 );
}

void SX128x::SetModulationParams(const ModulationParams_t& modParams )
{
	uint8_t buf[3];

	// Check if required configuration corresponds to the stored packet type
	// If not, silently update radio packet type
	if (this->PacketType != modParams.PacketType )
	{
		this->SetPacketType( modParams.PacketType );
	}

	switch( modParams.PacketType )
	{
		case PACKET_TYPE_GFSK:
			buf[0] = modParams.Params.Gfsk.BitrateBandwidth;
			buf[1] = modParams.Params.Gfsk.ModulationIndex;
			buf[2] = modParams.Params.Gfsk.ModulationShaping;
			break;
		case PACKET_TYPE_LORA:
		case PACKET_TYPE_RANGING:
			buf[0] = modParams.Params.LoRa.SpreadingFactor;
			buf[1] = modParams.Params.LoRa.Bandwidth;
			buf[2] = modParams.Params.LoRa.CodingRate;
			this->LoRaBandwidth = modParams.Params.LoRa.Bandwidth;
			break;
		case PACKET_TYPE_FLRC:
			buf[0] = modParams.Params.Flrc.BitrateBandwidth;
			buf[1] = modParams.Params.Flrc.CodingRate;
			buf[2] = modParams.Params.Flrc.ModulationShaping;
			break;
		case PACKET_TYPE_BLE:
			buf[0] = modParams.Params.Ble.BitrateBandwidth;
			buf[1] = modParams.Params.Ble.ModulationIndex;
			buf[2] = modParams.Params.Ble.ModulationShaping;
			break;
		case PACKET_TYPE_NONE:
			buf[0] = 0;
			buf[1] = 0;
			buf[2] = 0;
			break;
	}
	WriteCommand( RADIO_SET_MODULATIONPARAMS, buf, 3 );
	CurrentModParams = modParams;
}

void SX128x::SetPacketParams(const PacketParams_t& packetParams)
{
	uint8_t buf[7];
	// Check if required configuration corresponds to the stored packet type
	// If not, silently update radio packet type
	if (this->PacketType != packetParams.PacketType )
	{
		this->SetPacketType( packetParams.PacketType );
	}

	switch( packetParams.PacketType )
	{
		case PACKET_TYPE_GFSK:
			buf[0] = packetParams.Params.Gfsk.PreambleLength;
			buf[1] = packetParams.Params.Gfsk.SyncWordLength;
			buf[2] = packetParams.Params.Gfsk.SyncWordMatch;
			buf[3] = packetParams.Params.Gfsk.HeaderType;
			buf[4] = packetParams.Params.Gfsk.PayloadLength;
			buf[5] = packetParams.Params.Gfsk.CrcLength;
			buf[6] = packetParams.Params.Gfsk.Whitening;
			break;
		case PACKET_TYPE_LORA:
		case PACKET_TYPE_RANGING:
			buf[0] = packetParams.Params.LoRa.PreambleLength;
			buf[1] = packetParams.Params.LoRa.HeaderType;
			buf[2] = packetParams.Params.LoRa.PayloadLength;
			buf[3] = packetParams.Params.LoRa.Crc;
			buf[4] = packetParams.Params.LoRa.InvertIQ;
			buf[5] = 0;
			buf[6] = 0;
			break;
		case PACKET_TYPE_FLRC:
			buf[0] = packetParams.Params.Flrc.PreambleLength;
			buf[1] = packetParams.Params.Flrc.SyncWordLength;
			buf[2] = packetParams.Params.Flrc.SyncWordMatch;
			buf[3] = packetParams.Params.Flrc.HeaderType;
			buf[4] = packetParams.Params.Flrc.PayloadLength;
			buf[5] = packetParams.Params.Flrc.CrcLength;
			buf[6] = packetParams.Params.Flrc.Whitening;
			break;
		case PACKET_TYPE_BLE:
			buf[0] = packetParams.Params.Ble.ConnectionState;
			buf[1] = packetParams.Params.Ble.CrcLength;
			buf[2] = packetParams.Params.Ble.BleTestPayload;
			buf[3] = packetParams.Params.Ble.Whitening;
			buf[4] = 0;
			buf[5] = 0;
			buf[6] = 0;
			break;
		case PACKET_TYPE_NONE:
			buf[0] = 0;
			buf[1] = 0;
			buf[2] = 0;
			buf[3] = 0;
			buf[4] = 0;
			buf[5] = 0;
			buf[6] = 0;
			break;
	}
	WriteCommand( RADIO_SET_PACKETPARAMS, buf, 7 );
	CurrentPacketParams = packetParams;
}

void SX128x::ForcePreambleLength(RadioPreambleLengths_t preambleLength )
{
	this->WriteRegister( REG_LR_PREAMBLELENGTH, ( this->ReadRegister( REG_LR_PREAMBLELENGTH ) & MASK_FORCE_PREAMBLELENGTH ) | preambleLength );
}

void SX128x::GetRxBufferStatus(uint8_t *rxPayloadLength, uint8_t *rxStartBufferPointer )
{
	uint8_t status[2];

	ReadCommand( RADIO_GET_RXBUFFERSTATUS, status, 2 );

	// In case of LORA fixed header, the rxPayloadLength is obtained by reading
	// the register REG_LR_PAYLOADLENGTH
	if (( this -> GetPacketType( true ) == PACKET_TYPE_LORA ) && ( ReadRegister( REG_LR_PACKETPARAMS ) >> 7 == 1 ) )
	{
		*rxPayloadLength = ReadRegister( REG_LR_PAYLOADLENGTH );
	}
	else if (this -> GetPacketType( true ) == PACKET_TYPE_BLE )
	{
		// In the case of BLE, the size returned in status[0] do not include the 2-byte length PDU header
		// so it is added there
		*rxPayloadLength = status[0] + 2;
	}
	else
	{
		*rxPayloadLength = status[0];
	}

	*rxStartBufferPointer = status[1];
}

void SX128x::GetPacketStatus(PacketStatus_t *packetStatus )
{
	uint8_t status[5];

	ReadCommand( RADIO_GET_PACKETSTATUS, status, 5 );

	packetStatus->packetType = this -> GetPacketType( true );
	switch( packetStatus->packetType )
	{
		case PACKET_TYPE_GFSK:
			packetStatus->Gfsk.RssiSync = -( status[1] / 2 );

			packetStatus->Gfsk.ErrorStatus.SyncError = ( status[2] >> 6 ) & 0x01;
			packetStatus->Gfsk.ErrorStatus.LengthError = ( status[2] >> 5 ) & 0x01;
			packetStatus->Gfsk.ErrorStatus.CrcError = ( status[2] >> 4 ) & 0x01;
			packetStatus->Gfsk.ErrorStatus.AbortError = ( status[2] >> 3 ) & 0x01;
			packetStatus->Gfsk.ErrorStatus.HeaderReceived = ( status[2] >> 2 ) & 0x01;
			packetStatus->Gfsk.ErrorStatus.PacketReceived = ( status[2] >> 1 ) & 0x01;
			packetStatus->Gfsk.ErrorStatus.PacketControlerBusy = status[2] & 0x01;

			packetStatus->Gfsk.TxRxStatus.RxNoAck = ( status[3] >> 5 ) & 0x01;
			packetStatus->Gfsk.TxRxStatus.PacketSent = status[3] & 0x01;

			packetStatus->Gfsk.SyncAddrStatus = status[4] & 0x07;
			break;

		case PACKET_TYPE_LORA:
		case PACKET_TYPE_RANGING:
			packetStatus->LoRa.RssiPkt = -( status[0] / 2 );
			( status[1] < 128 ) ? ( packetStatus->LoRa.SnrPkt = status[1] / 4 ) : ( packetStatus->LoRa.SnrPkt = ( ( status[1] - 256 ) /4 ) );
			break;

		case PACKET_TYPE_FLRC:
			packetStatus->Flrc.RssiSync = -( status[1] / 2 );

			packetStatus->Flrc.ErrorStatus.SyncError = ( status[2] >> 6 ) & 0x01;
			packetStatus->Flrc.ErrorStatus.LengthError = ( status[2] >> 5 ) & 0x01;
			packetStatus->Flrc.ErrorStatus.CrcError = ( status[2] >> 4 ) & 0x01;
			packetStatus->Flrc.ErrorStatus.AbortError = ( status[2] >> 3 ) & 0x01;
			packetStatus->Flrc.ErrorStatus.HeaderReceived = ( status[2] >> 2 ) & 0x01;
			packetStatus->Flrc.ErrorStatus.PacketReceived = ( status[2] >> 1 ) & 0x01;
			packetStatus->Flrc.ErrorStatus.PacketControlerBusy = status[2] & 0x01;

			packetStatus->Flrc.TxRxStatus.RxPid = ( status[3] >> 6 ) & 0x03;
			packetStatus->Flrc.TxRxStatus.RxNoAck = ( status[3] >> 5 ) & 0x01;
			packetStatus->Flrc.TxRxStatus.RxPidErr = ( status[3] >> 4 ) & 0x01;
			packetStatus->Flrc.TxRxStatus.PacketSent = status[3] & 0x01;

			packetStatus->Flrc.SyncAddrStatus = status[4] & 0x07;
			break;

		case PACKET_TYPE_BLE:
			packetStatus->Ble.RssiSync =  -( status[1] / 2 );

			packetStatus->Ble.ErrorStatus.SyncError = ( status[2] >> 6 ) & 0x01;
			packetStatus->Ble.ErrorStatus.LengthError = ( status[2] >> 5 ) & 0x01;
			packetStatus->Ble.ErrorStatus.CrcError = ( status[2] >> 4 ) & 0x01;
			packetStatus->Ble.ErrorStatus.AbortError = ( status[2] >> 3 ) & 0x01;
			packetStatus->Ble.ErrorStatus.HeaderReceived = ( status[2] >> 2 ) & 0x01;
			packetStatus->Ble.ErrorStatus.PacketReceived = ( status[2] >> 1 ) & 0x01;
			packetStatus->Ble.ErrorStatus.PacketControlerBusy = status[2] & 0x01;

			packetStatus->Ble.TxRxStatus.PacketSent = status[3] & 0x01;

			packetStatus->Ble.SyncAddrStatus = status[4] & 0x07;
			break;

		case PACKET_TYPE_NONE:
			// In that specific case, we set everything in the packetStatus to zeros
			// and reset the packet type accordingly
			memset( packetStatus, 0, sizeof( PacketStatus_t ) );
			packetStatus->packetType = PACKET_TYPE_NONE;
			break;
	}
}

int8_t SX128x::GetRssiInst(void )
{
	uint8_t raw = 0;

	ReadCommand( RADIO_GET_RSSIINST, &raw, 1 );

	return ( int8_t ) ( -raw / 2 );
}

void SX128x::SetDioIrqParams(uint16_t irqMask, uint16_t dio1Mask, uint16_t dio2Mask, uint16_t dio3Mask )
{
	uint8_t buf[8];

	buf[0] = ( uint8_t )( ( irqMask >> 8 ) & 0x00FF );
	buf[1] = ( uint8_t )( irqMask & 0x00FF );
	buf[2] = ( uint8_t )( ( dio1Mask >> 8 ) & 0x00FF );
	buf[3] = ( uint8_t )( dio1Mask & 0x00FF );
	buf[4] = ( uint8_t )( ( dio2Mask >> 8 ) & 0x00FF );
	buf[5] = ( uint8_t )( dio2Mask & 0x00FF );
	buf[6] = ( uint8_t )( ( dio3Mask >> 8 ) & 0x00FF );
	buf[7] = ( uint8_t )( dio3Mask & 0x00FF );
	WriteCommand( RADIO_SET_DIOIRQPARAMS, buf, 8 );
}

uint16_t SX128x::GetIrqStatus(void )
{
	uint8_t irqStatus[2];
	ReadCommand( RADIO_GET_IRQSTATUS, irqStatus, 2 );
	return ( irqStatus[0] << 8 ) | irqStatus[1];
}

void SX128x::ClearIrqStatus(uint16_t irqMask )
{
	uint8_t buf[2];

	buf[0] = ( uint8_t )( ( ( uint16_t )irqMask >> 8 ) & 0x00FF );
	buf[1] = ( uint8_t )( ( uint16_t )irqMask & 0x00FF );
	WriteCommand( RADIO_CLR_IRQSTATUS, buf, 2 );
}

void SX128x::Calibrate(CalibrationParams_t calibParam )
{
	uint8_t cal = ( calibParam.ADCBulkPEnable << 5 ) |
		      ( calibParam.ADCBulkNEnable << 4 ) |
		      ( calibParam.ADCPulseEnable << 3 ) |
		      ( calibParam.PLLEnable << 2 ) |
		      ( calibParam.RC13MEnable << 1 ) |
		      ( calibParam.RC64KEnable );
	WriteCommand( RADIO_CALIBRATE, &cal, 1 );
}

void SX128x::SetRegulatorMode(RadioRegulatorModes_t mode ) // can be used
{
	WriteCommand( RADIO_SET_REGULATORMODE, ( uint8_t* )&mode, 1 );
}

void SX128x::SetSaveContext(void )
{
	WriteCommand( RADIO_SET_SAVECONTEXT, 0, 0 );
}

void SX128x::SetAutoFs(bool enableAutoFs ) // important
{
	WriteCommand( RADIO_SET_AUTOFS, ( uint8_t * )&enableAutoFs, 1 );
}

void SX128x::SetLongPreamble(bool enable )
{
	WriteCommand( RADIO_SET_LONGPREAMBLE, ( uint8_t * )&enable, 1 );
}

void SX128x::SetPayload(uint8_t *buffer, uint8_t size, uint8_t offset )
{
	WriteBuffer( offset, buffer, size );
}

uint8_t SX128x::GetPayload(uint8_t *buffer, uint8_t *size , uint8_t maxSize )
{
	uint8_t offset;

	GetRxBufferStatus( size, &offset );
	if (*size > maxSize )
	{
		return 1;
	}
	ReadBuffer( offset, buffer, *size );
	return 0;
}

void SX128x::SendPayload(uint8_t *payload, uint8_t size, TickTime_t timeout, uint8_t offset )
{
	SetPayload( payload, size, offset );
	SetTx( timeout );
}

double SX128x::GetFrequencyError( )
{
	uint8_t efeRaw[3] = {0};
	uint32_t efe = 0;
	double efeHz = 0.0;

	switch( this->GetPacketType( true ) )
	{
		case PACKET_TYPE_LORA:
		case PACKET_TYPE_RANGING:
			efeRaw[0] = this->ReadRegister( REG_LR_ESTIMATED_FREQUENCY_ERROR_MSB );
			efeRaw[1] = this->ReadRegister( REG_LR_ESTIMATED_FREQUENCY_ERROR_MSB + 1 );
			efeRaw[2] = this->ReadRegister( REG_LR_ESTIMATED_FREQUENCY_ERROR_MSB + 2 );
			efe = ( efeRaw[0]<<16 ) | ( efeRaw[1]<<8 ) | efeRaw[2];
			efe &= REG_LR_ESTIMATED_FREQUENCY_ERROR_MASK;

			efeHz = 1.55 * ( double )complement2( efe, 20 ) / ( 1600.0 / ( double )this->GetLoRaBandwidth( ) * 1000.0 );
			break;

		case PACKET_TYPE_NONE:
		case PACKET_TYPE_BLE:
		case PACKET_TYPE_FLRC:
		case PACKET_TYPE_GFSK:
			break;
	}

	return efeHz;
}

int32_t SX128x::complement2(const uint32_t num, const uint8_t bitCnt )
{
	int32_t retVal = ( int32_t )num;
	if (num >= 2<<( bitCnt - 2 ) )
	{
		retVal -= 2<<( bitCnt - 1 );
	}
	return retVal;
}

int32_t SX128x::GetLoRaBandwidth( )
{
	int32_t bwValue = 0;

	switch( this->LoRaBandwidth )
	{
		case LORA_BW_0200:
			bwValue = 203125;
			break;
		case LORA_BW_0400:
			bwValue = 406250;
			break;
		case LORA_BW_0800:
			bwValue = 812500;
			break;
		case LORA_BW_1600:
			bwValue = 1625000;
			break;
		default:
			bwValue = 0;
	}
	return bwValue;
}

void SX128x::ProcessIrqs() {
	RadioPacketTypes_t packetType = PACKET_TYPE_NONE;

	packetType = GetPacketType( true );
	uint16_t irqRegs = GetIrqStatus();
	ClearIrqStatus( IRQ_RADIO_ALL );

	auto& txDone = callbacks.txDone;
	auto& rxDone = callbacks.rxDone;
	auto& rxSyncWordDone = callbacks.rxSyncWordDone;
	auto& rxHeaderDone = callbacks.rxHeaderDone;
	auto& txTimeout = callbacks.txTimeout;
	auto& rxTimeout = callbacks.rxTimeout;
	auto& rxError = callbacks.rxError;
	auto& rangingDone = callbacks.rangingDone;
	auto& cadDone = callbacks.cadDone;

	switch( packetType )
	{	// only LORA is used
		case PACKET_TYPE_LORA:
			switch( OperatingMode )
			{
				case MODE_RX:
					if (( irqRegs & IRQ_RX_DONE ) == IRQ_RX_DONE )
					{
						// if (( irqRegs & IRQ_CRC_ERROR ) == IRQ_CRC_ERROR )
						// {
						// 	if (rxError)
						// 		rxError(IRQ_CRC_ERROR_CODE);
						// }
						// else
						// {
						// 	if (rxDone)
						// 		rxDone();
						// }
						rxDone();
					}
					if (( irqRegs & IRQ_HEADER_VALID ) == IRQ_HEADER_VALID )
					{
						if (rxHeaderDone)
							rxHeaderDone();
					}
					if (( irqRegs & IRQ_HEADER_ERROR ) == IRQ_HEADER_ERROR )
					{
						if (rxError)
							rxError( IRQ_HEADER_ERROR_CODE);
					}
					if (( irqRegs & IRQ_RX_TX_TIMEOUT ) == IRQ_RX_TX_TIMEOUT )
					{
						HalPostRx();
						if (rxTimeout)
							rxTimeout();
					}
					if (( irqRegs & IRQ_RANGING_SLAVE_REQUEST_DISCARDED ) == IRQ_RANGING_SLAVE_REQUEST_DISCARDED )
					{
						if (rxError)
							rxError( IRQ_RANGING_ON_LORA_ERROR_CODE);
					}
					break;
				case MODE_TX:
					if (( irqRegs & IRQ_TX_DONE ) == IRQ_TX_DONE )
					{
						HalPostTx();
						if (txDone)
							txDone();
					}
					if (( irqRegs & IRQ_RX_TX_TIMEOUT ) == IRQ_RX_TX_TIMEOUT )
					{
						HalPostTx();
						if (txTimeout)
							txTimeout();
					}
					break;
				default:
					// Unexpected IRQ: silently returns
					break;
			}
			break;
		default:
			// Unexpected IRQ: silently returns
			break;
	}
}


void SX128x::HalSpiRead(uint8_t *buffer_in, uint16_t size) {
	uint8_t useless[size];
	memset(useless, 0, size);
	HalSpiTransfer(buffer_in, useless, size);
}

void SX128x::HalSpiWrite(const uint8_t *buffer_out, uint16_t size) {
	uint8_t useless[size];
	HalSpiTransfer(useless, buffer_out, size);
}

inline void SX128x::WaitOnBusy() {
	while (HalGpioRead(GPIO_PIN_BUSY));
//	HAL_Delay(1);
//	for(int i = 0; i < 100; i++);
}

inline void SX128x::WaitOnBusyLong() {
	while (HalGpioRead(GPIO_PIN_BUSY))
		HAL_Delay(10);
}

void SX128x::Reset(void) {
	HalGpioWrite(GPIO_PIN_RESET, 1);
	HAL_Delay(20);
	HalGpioWrite(GPIO_PIN_RESET, 0);
	HAL_Delay(50);
	HalGpioWrite(GPIO_PIN_RESET, 1);
	HAL_Delay(20);
}

void SX128x::Wakeup(void) {
	uint8_t buf[2] = {RADIO_GET_STATUS, 0};
	HalSpiWrite(buf, 2);
	WaitOnBusyLong();
}

void SX128x::WriteCommand(SX128x::RadioCommands_t opcode, uint8_t *buffer, uint16_t size) {
	auto *merged_buf = (uint8_t *)alloca(size+1);

	merged_buf[0] = opcode;
	memcpy(merged_buf+1, buffer, size);

	WaitOnBusy();
	HalSpiWrite(merged_buf, size+1);
}

void SX128x::ReadCommand(SX128x::RadioCommands_t opcode, uint8_t *buffer, uint16_t size) {

	if (opcode == RADIO_GET_STATUS) {
		uint8_t buf_out[3] = {static_cast<uint8_t>(opcode), 0, 0};
		uint8_t buf_in[3];

		WaitOnBusy();
		HalSpiTransfer(buf_in, buf_out, 3);
		buffer[0] = buf_in[0];
	} else {
		auto total_transfer_size = 2+size;

		auto *buf_out = (uint8_t *)alloca(total_transfer_size);
		auto *buf_in = (uint8_t *)alloca(total_transfer_size);

		memset(buf_out, 0, total_transfer_size);
		buf_out[0] = opcode;

		WaitOnBusy(); // wait until not busy before spi transfer
		HalSpiTransfer(buf_in, buf_out, total_transfer_size);
		memcpy(buffer, buf_in+2, size);
	}
}

void SX128x::WriteRegister(uint16_t address, uint8_t *buffer, uint16_t size) {
	auto total_transfer_size = 3+size;
	auto *buf_out = (uint8_t *)alloca(total_transfer_size);

	buf_out[0] = RADIO_WRITE_REGISTER;
	buf_out[1] = ((address & 0xFF00) >> 8);
	buf_out[2] = (address & 0x00FF);
	memcpy(buf_out+3, buffer, size);

	WaitOnBusy(); // wait until not busy before spi transfer
	HalSpiWrite(buf_out, total_transfer_size);
}

inline void SX128x::WriteRegister(uint16_t address, uint8_t value) {
	WriteRegister(address, &value, 1);
}

void SX128x::ReadRegister(uint16_t address, uint8_t *buffer, uint16_t size) {
	
	auto total_transfer_size = 4+size;
	auto *buf_out = (uint8_t *)alloca(total_transfer_size);
	auto *buf_in = (uint8_t *)alloca(total_transfer_size);

	memset(buf_out, 0, total_transfer_size);
	buf_out[0] = RADIO_READ_REGISTER;
	buf_out[1] = ((address & 0xFF00) >> 8);
	buf_out[2] = (address & 0x00FF);

	WaitOnBusy(); // wait until not busy before spi transfer
	HalSpiTransfer(buf_in, buf_out, total_transfer_size);

	memcpy(buffer, buf_in+4, size);
}

inline uint8_t SX128x::ReadRegister(uint16_t address) {
	uint8_t data;
	ReadRegister( address, &data, 1 );
	return data;
}

void SX128x::WriteBuffer(uint8_t offset, uint8_t *buffer, uint8_t size) {
	auto total_transfer_size = 2+size;
	auto *buf_out = (uint8_t *)alloca(total_transfer_size);

	buf_out[0] = RADIO_WRITE_BUFFER;
	buf_out[1] = offset;

	memcpy(buf_out+2, buffer, size);
	
	WaitOnBusy();
	HalSpiWrite(buf_out, total_transfer_size);
}

void SX128x::ReadBuffer(uint8_t offset, uint8_t *buffer, uint8_t size) {
	auto total_transfer_size = 3+size;
	auto *buf_out = (uint8_t *)alloca(total_transfer_size);
	auto *buf_in = (uint8_t *)alloca(total_transfer_size);

	memset(buf_out, 0, total_transfer_size);

	buf_out[0] = RADIO_READ_BUFFER;
	buf_out[1] = offset;

	WaitOnBusy();
	HalSpiTransfer(buf_in, buf_out, total_transfer_size);

	memcpy(buffer, buf_in+3, size);
}
