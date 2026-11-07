/*
 * Copyright (c) 2013 L2jMobius
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR
 * IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
package org.l2jmobius.gameserver.network;

import java.sql.Connection;
import java.sql.PreparedStatement;
import java.sql.ResultSet;
import java.util.List;
import java.util.concurrent.locks.ReentrantLock;
import java.util.logging.Level;
import java.util.logging.Logger;

import org.l2jmobius.commons.database.DatabaseFactory;
import org.l2jmobius.commons.network.Client;
import org.l2jmobius.commons.network.buffer.ReadBuffer;
import org.l2jmobius.commons.network.buffer.WriteBuffer;
import org.l2jmobius.commons.threads.ThreadPool;
import org.l2jmobius.commons.util.TraceUtil;
import org.l2jmobius.gameserver.LoginServerThread;
import org.l2jmobius.gameserver.LoginServerThread.SessionKey;
import org.l2jmobius.gameserver.config.GeneralConfig;
import org.l2jmobius.gameserver.config.PlayerConfig;
import org.l2jmobius.gameserver.config.ServerConfig;
import org.l2jmobius.gameserver.config.custom.WeddingConfig;
import org.l2jmobius.gameserver.data.sql.CharacterAppearanceTable;
import org.l2jmobius.gameserver.data.sql.ClanTable;
import org.l2jmobius.gameserver.data.sql.PlayerInfoTable;
import org.l2jmobius.gameserver.data.sql.SummonInfoTable;
import org.l2jmobius.gameserver.data.xml.SecondaryAuthData;
import org.l2jmobius.gameserver.entity.World;
import org.l2jmobius.gameserver.entity.actor.Player;
import org.l2jmobius.gameserver.entity.clan.Clan;
import org.l2jmobius.gameserver.mechanics.html.protection.HtmlBypassProtection;
import org.l2jmobius.gameserver.network.holders.CharacterInfoHolder;
import org.l2jmobius.gameserver.network.holders.ClientHardwareInfoHolder;
import org.l2jmobius.gameserver.network.serverpackets.AbstractHtmlPacket;
import org.l2jmobius.gameserver.network.serverpackets.LeaveWorld;
import org.l2jmobius.gameserver.network.serverpackets.ServerPacket;
import org.l2jmobius.gameserver.network.serverpackets.SystemMessage;
import org.l2jmobius.gameserver.security.SecondaryPasswordAuth;
import org.l2jmobius.gameserver.util.FloodProtectors;

/**
 * Represents a client connected to the {@code GameServer}.<br>
 * This class handles the network communication and session state for an active player.<br>
 * It serves as the primary interface between the server logic and the remote client.
 * @author KenM, Mobius
 */
public class GameClient extends Client<org.l2jmobius.commons.network.Connection<GameClient>>
{
	private static final Logger LOGGER = Logger.getLogger(GameClient.class.getName());
	private static final Logger LOGGER_ACCOUNTING = Logger.getLogger("accounting");
	
	private final HtmlBypassProtection _htmlBypassProtection = new HtmlBypassProtection();
	private final FloodProtectors _floodProtectors = new FloodProtectors(this);
	private final ReentrantLock _playerLock = new ReentrantLock();
	private ConnectionState _connectionState = ConnectionState.CONNECTED;
	private Encryption _encryption = null;
	private String _ip = "N/A";
	private String _accountName;
	private SessionKey _sessionKey;
	private Player _player;
	private SecondaryPasswordAuth _secondaryAuth;
	private volatile ClientHardwareInfoHolder _hardwareInfo;
	private List<CharacterInfoHolder> _charSlotMapping = null;
	private volatile boolean _isDetached = false;
	private boolean _isAuthedGG;
	private boolean _protocolOk;
	private int _protocolVersion;
	private int[][] _trace;
	
	/**
	 * Creates a new instance of {@code GameClient}.<br>
	 * This constructor initializes the client with a network connection.<br>
	 * It also captures the remote IP address from the provided connection.
	 * @param connection The network connection associated with this client.
	 */
	public GameClient(org.l2jmobius.commons.network.Connection<GameClient> connection)
	{
		super(connection);
		_ip = connection.getRemoteAddress();
	}
	
	/**
	 * This method is called when a new client successfully connects.<br>
	 * It logs the connection event for the specific IP address.
	 */
	@Override
	public void onConnected()
	{
		LOGGER_ACCOUNTING.finer("Client connected: " + _ip);
	}
	
	/**
	 * This method is called when a client disconnects from the server.<br>
	 * It notifies the {@code LoginServerThread} to send a logout message.<br>
	 * It triggers the disconnection logic if the player is not in offline mode.<br>
	 * The connection state is updated to {@code DISCONNECTED}.
	 */
	@Override
	public void onDisconnection()
	{
		LOGGER_ACCOUNTING.finer("Client disconnected: " + this);
		LoginServerThread.getInstance().sendLogout(_accountName);
		if ((_player == null) || !_player.getOfflineMode().isEnabled())
		{
			Disconnection.of(this).onDisconnection();
		}
		
		_connectionState = ConnectionState.DISCONNECTED;
	}
	
	/**
	 * Encrypts the specified portion of a {@code WriteBuffer}.<br>
	 * This method checks if packet encryption is enabled in the configuration.<br>
	 * It applies the encryption to the data provided.
	 * @param data The {@code WriteBuffer} containing the data to be encrypted.
	 * @param offset The starting position within the buffer.
	 * @param size The number of bytes to encrypt.
	 * @return Always returns {@code true}.
	 */
	@Override
	public boolean encrypt(WriteBuffer data, int offset, int size)
	{
		if (ServerConfig.PACKET_ENCRYPTION && (_encryption != null))
		{
			_encryption.encrypt(data, offset, size);
		}
		
		return true;
	}
	
	/**
	 * Decrypts the provided data buffer.<br>
	 * This method checks if packet encryption is enabled in {@code ServerConfig}.<br>
	 * It then applies decryption to the specified range of the {@code ReadBuffer}.
	 * @param data The {@code ReadBuffer} containing the encrypted data.
	 * @param offset The starting position in the buffer.
	 * @param size The number of bytes to decrypt.
	 * @return Always returns {@code true}.
	 */
	@Override
	public boolean decrypt(ReadBuffer data, int offset, int size)
	{
		if (ServerConfig.PACKET_ENCRYPTION && (_encryption != null))
		{
			_encryption.decrypt(data, offset, size);
		}
		
		return true;
	}
	
	/**
	 * Closes the connection immediately.<br>
	 * This method calls {@code disconnect()} to terminate the session.
	 */
	public void closeNow()
	{
		disconnect();
	}
	
	/**
	 * Closes the current connection.<br>
	 * If {@code packet} is {@code null}, it closes the connection immediately.<br>
	 * Otherwise, it sends the provided {@code packet} and schedules a close after 1000ms.
	 * @param packet The packet to send before closing the connection.
	 */
	public void close(ServerPacket packet)
	{
		if (packet == null)
		{
			closeNow();
		}
		else
		{
			// Send the close packet.
			sendPacket(packet);
			
			// Wait for packet to be sent.
			ThreadPool.schedule(this::closeNow, 1000);
		}
	}
	
	/**
	 * Enables packet encryption for the current session.<br>
	 * This method generates a random key using {@code BlowFishKeygen}.<br>
	 * It initializes the {@code Encryption} object if {@code ServerConfig.PACKET_ENCRYPTION} is true.
	 * @return The generated encryption key as a {@code byte[]} array.
	 */
	public byte[] enableCrypt()
	{
		final byte[] key = BlowFishKeygen.getRandomKey();
		if (ServerConfig.PACKET_ENCRYPTION)
		{
			_encryption = new Encryption();
			_encryption.setKey(key);
		}
		
		return key;
	}
	
	/**
	 * Retrieves the {@code Player} associated with this client.
	 * @return the current {@code Player} object.
	 */
	public Player getPlayer()
	{
		return _player;
	}
	
	/**
	 * Sets the {@code Player} associated with this client.<br>
	 * This method updates the internal {@code _player} field.
	 * @param player The {@code Player} object to set.
	 */
	public void setPlayer(Player player)
	{
		_player = player;
	}
	
	/**
	 * Retrieves the {@code ReentrantLock} associated with this player.<br>
	 * This lock is used to ensure thread safety when modifying player data.
	 * @return the {@code ReentrantLock} for the player.
	 */
	public ReentrantLock getPlayerLock()
	{
		return _playerLock;
	}
	
	/**
	 * Retrieves the flood protection settings for this client.<br>
	 * This allows you to check the current limits and rules applied to the connection.
	 * @return the {@code FloodProtectors} object associated with this client.
	 */
	public FloodProtectors getFloodProtectors()
	{
		return _floodProtectors;
	}
	
	/**
	 * Updates the GameGuard authentication status.<br>
	 * This method sets whether the client has successfully passed GameGuard checks.
	 * @param value The new authentication status to set.
	 */
	public void setGameGuardOk(boolean value)
	{
		_isAuthedGG = value;
	}
	
	/**
	 * Checks if the client has authenticated for GameGuard.
	 * @return {@code true} if the client is authenticated, {@code false} otherwise.
	 */
	public boolean isAuthedGG()
	{
		return _isAuthedGG;
	}
	
	/**
	 * Retrieves the IP address of the connected client.
	 * @return The {@code String} representation of the client's IP address.
	 */
	public String getIp()
	{
		return _ip;
	}
	
	/**
	 * Sets the name of the account for this client.<br>
	 * This method also initializes secondary authentication if it is enabled in {@code SecondaryAuthData}.
	 * @param accountName The name of the account to set.
	 */
	public void setAccountName(String accountName)
	{
		_accountName = accountName;
		if (SecondaryAuthData.getInstance().isEnabled())
		{
			_secondaryAuth = new SecondaryPasswordAuth(this);
		}
	}
	
	/**
	 * Retrieves the name of the account associated with this client.
	 * @return The {@code String} representing the account name.
	 */
	public String getAccountName()
	{
		return _accountName;
	}
	
	/**
	 * Sets the unique session key for this client.<br>
	 * This identifies the current connection during the game session.
	 * @param sessionKey The {@code SessionKey} to assign to this client.
	 */
	public void setSessionId(SessionKey sessionKey)
	{
		_sessionKey = sessionKey;
	}
	
	/**
	 * Retrieves the unique session key for this client.<br>
	 * This key is used to identify the current connection.
	 * @return the {@code SessionKey} associated with this session.
	 */
	public SessionKey getSessionId()
	{
		return _sessionKey;
	}
	
	/**
	 * Retrieves the html bypass protection of this client.
	 * @return The {@code HtmlBypassProtection} holding the encrypted bypasses sent to this client.
	 */
	public HtmlBypassProtection getHtmlBypassProtection()
	{
		return _htmlBypassProtection;
	}
	
	/**
	 * Sends a {@code ServerPacket} to the connected client.<br>
	 * This method validates that the {@code packet} is not {@code null}.<br>
	 * It writes the data and then executes the packet implementation.
	 * @param packet The {@code ServerPacket} to be sent.
	 */
	public void sendPacket(ServerPacket packet)
	{
		// Packet should never be null.
		if (packet == null)
		{
			LOGGER.warning(TraceUtil.getStackTrace(new Exception()));
			return;
		}
		
		// Html bypass protection.
		if (GeneralConfig.HTML_BYPASS_PROTECTION && (packet instanceof AbstractHtmlPacket))
		{
			((AbstractHtmlPacket) packet).protectBypasses(this);
		}
		
		// Send the packet data.
		writePacket(packet);
		
		// Run packet implementation.
		packet.runImpl(_player);
	}
	
	/**
	 * Sends a system message to the connected client.<br>
	 * This method creates a new {@code SystemMessage} using the provided ID.<br>
	 * It then uses the {@code sendPacket} method to transmit it.
	 * @param systemMessageId The unique identifier for the system message to be sent.
	 */
	public void sendPacket(SystemMessageId systemMessageId)
	{
		sendPacket(new SystemMessage(systemMessageId));
	}
	
	/**
	 * Checks if the client is currently in a detached state.<br>
	 * This indicates whether the connection is active but not fully engaged with the game world.
	 * @return {@code true} if the client is detached, {@code false} otherwise.
	 */
	public boolean isDetached()
	{
		return _isDetached;
	}
	
	/**
	 * Sets whether the client is currently detached from the server.<br>
	 * Use {@code true} to indicate a detached state.<br>
	 * Use {@code false} to indicate an attached state.
	 * @param value The desired detached status.
	 */
	public void setDetached(boolean value)
	{
		_isDetached = value;
	}
	
	/**
	 * Marks a character for deletion based on its slot.<br>
	 * This method checks the clan status and updates the deletion time in the database.<br>
	 * It returns different values depending on whether the character is a leader or a member.
	 * @param characterSlot The index of the character slot to process.
	 * @return A byte representing the result: 0 for no clan, 1 for a member, 2 for a leader, and -1 for an error.
	 */
	public byte markToDeleteChar(int characterSlot)
	{
		final int objectId = getObjectIdForSlot(characterSlot);
		if (objectId < 0)
		{
			return -1;
		}
		
		try (Connection con = DatabaseFactory.getConnection();
			PreparedStatement statement = con.prepareStatement("SELECT clanId FROM characters WHERE charId=?"))
		{
			statement.setInt(1, objectId);
			byte answer = 0;
			try (ResultSet rs = statement.executeQuery())
			{
				final int clanId = rs.next() ? rs.getInt(1) : 0;
				if (clanId != 0)
				{
					final Clan clan = ClanTable.getInstance().getClan(clanId);
					if (clan == null)
					{
						answer = 0; // jeezes!
					}
					else if (clan.getLeaderId() == objectId)
					{
						answer = 2;
					}
					else
					{
						answer = 1;
					}
				}
				
				// Setting delete time
				if (answer == 0)
				{
					if (PlayerConfig.DELETE_DAYS == 0)
					{
						deleteCharByObjId(objectId);
					}
					else
					{
						try (PreparedStatement ps2 = con.prepareStatement("UPDATE characters SET deletetime=? WHERE charId=?"))
						{
							ps2.setLong(1, System.currentTimeMillis() + (PlayerConfig.DELETE_DAYS * 86400000)); // 24*60*60*1000 = 86400000
							ps2.setInt(2, objectId);
							ps2.execute();
						}
					}
					
					LOGGER_ACCOUNTING.info("Delete, " + objectId + ", " + this);
				}
			}
			
			return answer;
		}
		catch (Exception e)
		{
			LOGGER.log(Level.SEVERE, "Error updating delete time of character.", e);
			return -1;
		}
	}
	
	/**
	 * Restores a deleted character from the database.<br>
	 * This method sets the {@code deletetime} to {@code 0} for the specified slot.<br>
	 * It logs an error if the database update fails.
	 * @param characterSlot The index of the character slot to restore.
	 */
	public void restore(int characterSlot)
	{
		final int objectId = getObjectIdForSlot(characterSlot);
		if (objectId < 0)
		{
			return;
		}
		
		try (Connection con = DatabaseFactory.getConnection();
			PreparedStatement statement = con.prepareStatement("UPDATE characters SET deletetime=0 WHERE charId=?"))
		{
			statement.setInt(1, objectId);
			statement.execute();
		}
		catch (Exception e)
		{
			LOGGER.log(Level.SEVERE, "Error restoring character.", e);
		}
		
		LOGGER_ACCOUNTING.info("Restore, " + objectId + ", " + this);
	}
	
	/**
	 * Deletes all data associated with a specific character from the database.<br>
	 * This method removes entries from multiple tables including skills, items, and friends.<br>
	 * It also clears information from {@code PlayerInfoTable} and {@code SummonInfoTable}.<br>
	 * If the provided {@code objectId} is less than {@code 0}, no action is taken.
	 * @param objectId The unique identifier of the character to be deleted.
	 */
	public static void deleteCharByObjId(int objectId)
	{
		if (objectId < 0)
		{
			return;
		}
		
		PlayerInfoTable.getInstance().removeName(objectId);
		SummonInfoTable.getInstance().removeCharacter(objectId);
		CharacterAppearanceTable.getInstance().delete(objectId);
		
		try (Connection con = DatabaseFactory.getConnection())
		{
			try (PreparedStatement ps = con.prepareStatement("DELETE FROM character_contacts WHERE charId=? OR contactId=?"))
			{
				ps.setInt(1, objectId);
				ps.setInt(2, objectId);
				ps.execute();
			}
			
			try (PreparedStatement ps = con.prepareStatement("DELETE FROM character_friends WHERE charId=? OR friendId=?"))
			{
				ps.setInt(1, objectId);
				ps.setInt(2, objectId);
				ps.execute();
			}
			
			try (PreparedStatement ps = con.prepareStatement("DELETE FROM character_hennas WHERE charId=?"))
			{
				ps.setInt(1, objectId);
				ps.execute();
			}
			
			try (PreparedStatement ps = con.prepareStatement("DELETE FROM character_macroses WHERE charId=?"))
			{
				ps.setInt(1, objectId);
				ps.execute();
			}
			
			try (PreparedStatement ps = con.prepareStatement("DELETE FROM character_quests WHERE charId=?"))
			{
				ps.setInt(1, objectId);
				ps.execute();
			}
			
			try (PreparedStatement ps = con.prepareStatement("DELETE FROM character_recipebook WHERE charId=?"))
			{
				ps.setInt(1, objectId);
				ps.execute();
			}
			
			try (PreparedStatement ps = con.prepareStatement("DELETE FROM character_shortcuts WHERE charId=?"))
			{
				ps.setInt(1, objectId);
				ps.execute();
			}
			
			try (PreparedStatement ps = con.prepareStatement("DELETE FROM character_skills WHERE charId=?"))
			{
				ps.setInt(1, objectId);
				ps.execute();
			}
			
			try (PreparedStatement ps = con.prepareStatement("DELETE FROM character_skills_save WHERE charId=?"))
			{
				ps.setInt(1, objectId);
				ps.execute();
			}
			
			try (PreparedStatement ps = con.prepareStatement("DELETE FROM character_subclasses WHERE charId=?"))
			{
				ps.setInt(1, objectId);
				ps.execute();
			}
			
			try (PreparedStatement ps = con.prepareStatement("DELETE FROM character_summons WHERE ownerId=?"))
			{
				ps.setInt(1, objectId);
				ps.execute();
			}
			
			try (PreparedStatement ps = con.prepareStatement("DELETE FROM heroes WHERE charId=?"))
			{
				ps.setInt(1, objectId);
				ps.execute();
			}
			
			try (PreparedStatement ps = con.prepareStatement("DELETE FROM olympiad_nobles WHERE charId=?"))
			{
				ps.setInt(1, objectId);
				ps.execute();
			}
			
			try (PreparedStatement ps = con.prepareStatement("DELETE FROM seven_signs WHERE charId=?"))
			{
				ps.setInt(1, objectId);
				ps.execute();
			}
			
			try (PreparedStatement ps = con.prepareStatement("DELETE FROM pets WHERE item_obj_id IN (SELECT object_id FROM items WHERE items.owner_id=?)"))
			{
				ps.setInt(1, objectId);
				ps.execute();
			}
			
			try (PreparedStatement ps = con.prepareStatement("DELETE FROM item_augmentations WHERE itemId IN (SELECT object_id FROM items WHERE items.owner_id=?)"))
			{
				ps.setInt(1, objectId);
				ps.execute();
			}
			
			try (PreparedStatement ps = con.prepareStatement("DELETE FROM items WHERE owner_id=?"))
			{
				ps.setInt(1, objectId);
				ps.execute();
			}
			
			try (PreparedStatement ps = con.prepareStatement("DELETE FROM merchant_lease WHERE player_id=?"))
			{
				ps.setInt(1, objectId);
				ps.execute();
			}
			
			try (PreparedStatement ps = con.prepareStatement("DELETE FROM character_raid_points WHERE charId=?"))
			{
				ps.setInt(1, objectId);
				ps.execute();
			}
			
			try (PreparedStatement ps = con.prepareStatement("DELETE FROM character_reco_bonus WHERE charId=?"))
			{
				ps.setInt(1, objectId);
				ps.execute();
			}
			
			try (PreparedStatement ps = con.prepareStatement("DELETE FROM character_instance_time WHERE charId=?"))
			{
				ps.setInt(1, objectId);
				ps.execute();
			}
			
			try (PreparedStatement ps = con.prepareStatement("DELETE FROM character_variables WHERE charId=?"))
			{
				ps.setInt(1, objectId);
				ps.execute();
			}
			
			try (PreparedStatement ps = con.prepareStatement("DELETE FROM characters WHERE charId=?"))
			{
				ps.setInt(1, objectId);
				ps.execute();
			}
			
			if (WeddingConfig.ALLOW_WEDDING)
			{
				try (PreparedStatement ps = con.prepareStatement("DELETE FROM mods_wedding WHERE player1Id = ? OR player2Id = ?"))
				{
					ps.setInt(1, objectId);
					ps.setInt(2, objectId);
					ps.execute();
				}
			}
		}
		catch (Exception e)
		{
			LOGGER.log(Level.SEVERE, "Error deleting character.", e);
		}
	}
	
	/**
	 * Loads a {@code Player} object from the database based on a specific character slot.<br>
	 * This method checks if the character is already online and handles potential double logins.<br>
	 * It returns the loaded {@code Player} or {@code null} if the load fails.
	 * @param characterSlot The index of the character slot to load from.
	 * @return The loaded {@code Player} object, or {@code null} if not found or already online.
	 */
	public Player load(int characterSlot)
	{
		final int objectId = getObjectIdForSlot(characterSlot);
		if (objectId < 0)
		{
			return null;
		}
		
		Player player = World.getPlayer(objectId);
		if (player != null)
		{
			// exploit prevention, should not happens in normal way
			if (player.isOnlineInt() == 1)
			{
				LOGGER.severe("Attempt of double login: " + player.getName() + "(" + objectId + ") " + _accountName);
			}
			
			if (player.getClient() != null)
			{
				Disconnection.of(player).storeAndDeleteWith(LeaveWorld.STATIC_PACKET);
			}
			else
			{
				player.storeMe();
				player.deleteMe();
			}
			
			return null;
		}
		
		player = Player.load(objectId);
		if (player != null)
		{
			// prevent some values for each login
			player.setRunning(); // running is default
			player.standUp(); // standing is default
			player.refreshOverloaded();
			player.refreshExpertisePenalty();
		}
		else
		{
			LOGGER.severe("Could not restore in slot: " + characterSlot);
		}
		
		return player;
	}
	
	/**
	 * Updates the mapping of character slots.<br>
	 * This method sets the internal list of {@code CharacterInfoHolder} objects.
	 * @param characters The list of {@code CharacterInfoHolder} to assign.
	 */
	public void setCharSelection(List<CharacterInfoHolder> characters)
	{
		_charSlotMapping = characters;
	}
	
	/**
	 * Retrieves the {@code CharacterInfoHolder} associated with a specific slot.<br>
	 * This method checks if the provided index is valid before returning the data.
	 * @param charslot The index of the character slot to retrieve.
	 * @return The {@code CharacterInfoHolder} for the given slot, or {@code null} if it does not exist.
	 */
	public CharacterInfoHolder getCharSelection(int charslot)
	{
		if ((_charSlotMapping == null) || (charslot < 0) || (charslot >= _charSlotMapping.size()))
		{
			return null;
		}
		
		return _charSlotMapping.get(charslot);
	}
	
	/**
	 * Retrieves the secondary authentication data for the current session.<br>
	 * This object contains information required to verify a second factor of authentication.
	 * @return the {@code SecondaryPasswordAuth} object associated with this client.
	 */
	public SecondaryPasswordAuth getSecondaryAuth()
	{
		return _secondaryAuth;
	}
	
	/**
	 * Retrieves the unique object ID for a character based on its slot.<br>
	 * This method checks if a character exists in the specified {@code characterSlot}.<br>
	 * It returns -1 if no character is found at that position.
	 * @param characterSlot The index of the character slot to check.
	 * @return The unique object ID of the character, or -1 if none exists.
	 */
	private int getObjectIdForSlot(int characterSlot)
	{
		final CharacterInfoHolder info = getCharSelection(characterSlot);
		if (info == null)
		{
			LOGGER.warning(toString() + " tried to delete Character in slot " + characterSlot + " but no characters exits at that slot.");
			return -1;
		}
		
		return info.getObjectId();
	}
	
	/**
	 * Updates the current state of the network connection.<br>
	 * This method sets the {@code _connectionState} field to the provided value.
	 * @param connectionState The new {@code ConnectionState} to apply.
	 */
	public void setConnectionState(ConnectionState connectionState)
	{
		_connectionState = connectionState;
	}
	
	/**
	 * Retrieves the current state of the network connection.<br>
	 * This value indicates whether the connection is active or closed.
	 * @return The current {@code ConnectionState}.
	 */
	public ConnectionState getConnectionState()
	{
		return _connectionState;
	}
	
	/**
	 * Sets the protocol version for the client.<br>
	 * This value is used to ensure compatibility between the server and the client.
	 * @param version The integer value of the protocol version.
	 */
	public void setProtocolVersion(int version)
	{
		_protocolVersion = version;
	}
	
	/**
	 * Retrieves the current protocol version.<br>
	 * This value is used to ensure compatibility between the client and server.
	 * @return The current protocol version as an {@code int}.
	 */
	public int getProtocolVersion()
	{
		return _protocolVersion;
	}
	
	/**
	 * Checks if the current network protocol is valid.<br>
	 * This method returns {@code true} if the protocol is correct.<br>
	 * It returns {@code false} otherwise.
	 * @return {@code true} if the protocol is okay, {@code false} otherwise.
	 */
	public boolean isProtocolOk()
	{
		return _protocolOk;
	}
	
	/**
	 * Sets whether the protocol status is valid.<br>
	 * This updates the {@code _protocolOk} flag for the current client.
	 * @param value The boolean state to set for the protocol status.
	 */
	public void setProtocolOk(boolean value)
	{
		_protocolOk = value;
	}
	
	/**
	 * Sets the trace route for the client.<br>
	 * This method updates the internal {@code _trace} field with the provided data.
	 * @param tracert The 2D integer array containing the trace route.
	 */
	public void setClientTracert(int[][] tracert)
	{
		_trace = tracert;
	}
	
	/**
	 * Retrieves the current trace data.
	 * @return a 2D array containing the trace information.
	 */
	public int[][] getTrace()
	{
		return _trace;
	}
	
	/**
	 * Retrieves the current {@code Encryption} object.<br>
	 * This provides access to the encryption settings used for this client.
	 * @return The {@code Encryption} instance.
	 */
	public Encryption getEncryption()
	{
		return _encryption;
	}
	
	/**
	 * Retrieves the hardware information of the client.
	 * @return a {@code ClientHardwareInfoHolder} object containing the hardware details.
	 */
	public ClientHardwareInfoHolder getHardwareInfo()
	{
		return _hardwareInfo;
	}
	
	/**
	 * Updates the hardware information for the current client.<br>
	 * This method stores the provided {@code ClientHardwareInfoHolder} object.
	 * @param hardwareInfo The hardware details to be stored.
	 */
	public void setHardwareInfo(ClientHardwareInfoHolder hardwareInfo)
	{
		_hardwareInfo = hardwareInfo;
	}
	
	/**
	 * Returns a string representation of the current client status.<br>
	 * This includes information like account name, character details, and IP address.<br>
	 * The format changes based on the connection state.
	 * @return A formatted string describing the client's current state.
	 */
	@Override
	public String toString()
	{
		try
		{
			final String ip = getIp();
			final ConnectionState state = getConnectionState();
			switch (state)
			{
				case DISCONNECTED:
				{
					if (_accountName != null)
					{
						return "[Account: " + _accountName + " - IP: " + (ip == null ? "disconnected" : ip) + "]";
					}
					
					return "[IP: " + (ip == null ? "disconnected" : ip) + "]";
				}
				case CONNECTED:
				{
					return "[IP: " + (ip == null ? "disconnected" : ip) + "]";
				}
				case AUTHENTICATED:
				{
					return "[Account: " + _accountName + " - IP: " + (ip == null ? "disconnected" : ip) + "]";
				}
				case ENTERING:
				case DISABLED:
				case IN_GAME:
				{
					return "[Character: " + (_player == null ? "disconnected" : _player.getName() + "[" + _player.getObjectId() + "]") + " - Account: " + _accountName + " - IP: " + (ip == null ? "disconnected" : ip) + "]";
				}
				default:
				{
					throw new IllegalStateException("Missing state on switch.");
				}
			}
		}
		catch (NullPointerException e)
		{
			return "[Character read failed due to disconnect]";
		}
	}
}
