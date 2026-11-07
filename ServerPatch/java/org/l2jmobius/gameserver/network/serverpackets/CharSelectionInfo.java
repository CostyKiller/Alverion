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
package org.l2jmobius.gameserver.network.serverpackets;

import java.sql.Connection;
import java.sql.PreparedStatement;
import java.sql.ResultSet;
import java.util.ArrayList;
import java.util.List;

import org.l2jmobius.commons.database.DatabaseFactory;
import org.l2jmobius.commons.network.buffer.WriteBuffer;
import org.l2jmobius.gameserver.config.ServerConfig;
import org.l2jmobius.gameserver.config.custom.MultilingualSupportConfig;
import org.l2jmobius.gameserver.config.custom.OfflinePlayConfig;
import org.l2jmobius.gameserver.config.custom.OfflineTradeConfig;
import org.l2jmobius.gameserver.data.sql.CharacterAppearanceTable;
import org.l2jmobius.gameserver.data.sql.ClanTable;
import org.l2jmobius.gameserver.data.xml.ExperienceData;
import org.l2jmobius.gameserver.entity.World;
import org.l2jmobius.gameserver.entity.actor.Player;
import org.l2jmobius.gameserver.entity.clan.Clan;
import org.l2jmobius.gameserver.entity.itemcontainer.Inventory;
import org.l2jmobius.gameserver.network.Disconnection;
import org.l2jmobius.gameserver.network.GameClient;
import org.l2jmobius.gameserver.network.PacketLogger;
import org.l2jmobius.gameserver.network.ServerPackets;
import org.l2jmobius.gameserver.network.holders.CharacterInfoHolder;

/**
 * This packet provides the client with information regarding a selected character.<br>
 * It is used to synchronize character data during the selection process.
 */
public class CharSelectionInfo extends ServerPacket
{
	private final String _loginName;
	private final int _sessionId;
	private int _activeId;
	private final List<CharacterInfoHolder> _characterPackages;
	
	/**
	 * Creates a new {@code CharSelectionInfo} object for a specific user.<br>
	 * This constructor loads all character data associated with the provided login name.
	 * @param loginName The unique name used to identify the account.
	 * @param sessionId The current network session identifier.
	 */
	public CharSelectionInfo(String loginName, int sessionId)
	{
		_sessionId = sessionId;
		_loginName = loginName;
		_characterPackages = loadCharacterSelectInfo(_loginName);
		_activeId = -1;
	}
	
	/**
	 * Creates a new {@code CharSelectionInfo} object with specific character data.<br>
	 * This constructor initializes the session and loads all characters for the given login name.
	 * @param loginName The unique name used to identify the user account.
	 * @param sessionId The current network session identifier.
	 * @param activeId The ID of the character currently selected by the user.
	 */
	public CharSelectionInfo(String loginName, int sessionId, int activeId)
	{
		_sessionId = sessionId;
		_loginName = loginName;
		_characterPackages = loadCharacterSelectInfo(_loginName);
		_activeId = activeId;
	}
	
	/**
	 * Retrieves the list of character information for the current session.<br>
	 * This method returns all loaded {@code CharacterInfoHolder} objects.
	 * @return a {@code List} containing {@code CharacterInfoHolder} data.
	 */
	public List<CharacterInfoHolder> getCharInfo()
	{
		return _characterPackages;
	}
	
	@Override
	public void writeImpl(GameClient client, WriteBuffer buffer)
	{
		ServerPackets.CHARACTER_SELECTION_INFO.writeId(this, buffer);
		final int size = _characterPackages.size();
		buffer.writeInt(size); // Created character count
		buffer.writeInt(ServerConfig.MAX_CHARACTERS_NUMBER_PER_ACCOUNT); // Can prevent players from creating new characters (if 0); (if 1, the client will ask if chars may be created (0x13) Response: (0x0D) ).
		buffer.writeByte(0);
		long lastAccess = 0;
		if (_activeId == -1)
		{
			for (int i = 0; i < size; i++)
			{
				if (lastAccess < _characterPackages.get(i).getLastAccess())
				{
					lastAccess = _characterPackages.get(i).getLastAccess();
					_activeId = i;
				}
			}
		}
		
		for (int i = 0; i < size; i++)
		{
			final CharacterInfoHolder charInfoPackage = _characterPackages.get(i);
			buffer.writeString(charInfoPackage.getName()); // Character name
			buffer.writeInt(charInfoPackage.getObjectId()); // Character ID
			buffer.writeString(_loginName); // Account name
			buffer.writeInt(_sessionId); // Account ID
			buffer.writeInt(charInfoPackage.getClanId()); // Clan ID
			buffer.writeInt(0); // Builder level
			buffer.writeInt(charInfoPackage.getSex()); // Sex
			buffer.writeInt(charInfoPackage.getRace()); // Race
			buffer.writeInt(charInfoPackage.getBaseClassId());
			buffer.writeInt(1); // GameServerName
			buffer.writeInt(charInfoPackage.getX());
			buffer.writeInt(charInfoPackage.getY());
			buffer.writeInt(charInfoPackage.getZ());
			buffer.writeDouble(charInfoPackage.getCurrentHp());
			buffer.writeDouble(charInfoPackage.getCurrentMp());
			buffer.writeInt((int) charInfoPackage.getSp());
			buffer.writeLong(charInfoPackage.getExp());
			buffer.writeDouble((float) (charInfoPackage.getExp() - ExperienceData.getInstance().getExpForLevel(charInfoPackage.getLevel())) / (ExperienceData.getInstance().getExpForLevel(charInfoPackage.getLevel() + 1) - ExperienceData.getInstance().getExpForLevel(charInfoPackage.getLevel()))); // High
			
			// Five
			buffer.writeInt(charInfoPackage.getLevel());
			buffer.writeInt(charInfoPackage.getKarma());
			buffer.writeInt(charInfoPackage.getPkKills());
			buffer.writeInt(charInfoPackage.getPvPKills());
			buffer.writeInt(0);
			buffer.writeInt(0);
			buffer.writeInt(0);
			buffer.writeInt(0);
			buffer.writeInt(0);
			buffer.writeInt(0);
			buffer.writeInt(0);
			for (int slot : getPaperdollOrder())
			{
				buffer.writeInt(charInfoPackage.getPaperdollItemId(slot));
			}
			
			buffer.writeInt(charInfoPackage.getHairStyle());
			buffer.writeInt(charInfoPackage.getHairColor());
			buffer.writeInt(charInfoPackage.getFace());
			buffer.writeDouble(charInfoPackage.getMaxHp()); // Maximum HP
			buffer.writeDouble(charInfoPackage.getMaxMp()); // Maximum MP
			
			if (charInfoPackage.getAccessLevel() > -1)
			{
				// Deleted.
				if (charInfoPackage.getDeleteTimer() > 0)
				{
					buffer.writeInt((int) ((charInfoPackage.getDeleteTimer() - System.currentTimeMillis()) / 1000));
				}
				else // Normal.
				{
					buffer.writeInt(0);
				}
			}
			else // Banned.
			{
				buffer.writeInt(-1);
			}
			
			buffer.writeInt(charInfoPackage.getClassId());
			buffer.writeInt(i == _activeId);
			buffer.writeByte(Math.min(charInfoPackage.getEnchantEffect(), 127));
			buffer.writeInt(charInfoPackage.getAugmentationId());
			// buffer.writeInt(charInfoPackage.getTransformId()); // Used to display Transformations
			buffer.writeInt(0); // Currently on retail when you are on character select you don't see your transformation.
			buffer.writeInt(0); // Pet NpcId
			buffer.writeInt(0); // Pet level
			buffer.writeInt(0); // Pet Food
			buffer.writeInt(0); // Pet Food Level
			buffer.writeDouble(0); // Current pet HP
			buffer.writeDouble(0); // Current pet MP
			buffer.writeInt(charInfoPackage.getVitalityPoints()); // Vitality
		}
		
		// Alverion appearance extension.
		final List<byte[]> appearances = new ArrayList<>(size);
		for (CharacterInfoHolder charInfoPackage : _characterPackages)
		{
			appearances.add(CharacterAppearanceTable.getInstance().get(charInfoPackage.getObjectId()));
		}
		
		CharacterAppearanceTable.writeTrailer(buffer, appearances);
	}
	
	/**
	 * Retrieves a list of characters associated with a specific login name.<br>
	 * This method queries the database and populates {@code CharacterInfoHolder} objects.<br>
	 * It also handles disconnecting offline players if configured in the settings.
	 * @param loginName The unique account name used to identify the characters.
	 * @return A {@code List} of {@code CharacterInfoHolder} objects found for the given account.
	 */
	private static List<CharacterInfoHolder> loadCharacterSelectInfo(String loginName)
	{
		CharacterInfoHolder charInfopackage;
		final List<CharacterInfoHolder> characterList = new ArrayList<>();
		try (Connection con = DatabaseFactory.getConnection();
			PreparedStatement statement = con.prepareStatement("SELECT * FROM characters WHERE account_name=? ORDER BY createDate"))
		{
			statement.setString(1, loginName);
			try (ResultSet charList = statement.executeQuery())
			{
				while (charList.next()) // fills the package
				{
					charInfopackage = restoreChar(charList);
					if (charInfopackage != null)
					{
						characterList.add(charInfopackage);
						
						// Disconnect offline trader.
						if (OfflineTradeConfig.OFFLINE_DISCONNECT_SAME_ACCOUNT)
						{
							final Player player = World.getPlayer(charInfopackage.getObjectId());
							if ((player != null) && player.isInStoreMode())
							{
								Disconnection.of(player).storeAndDelete();
								continue;
							}
						}
						
						// Disconnect offline play.
						if (OfflinePlayConfig.OFFLINE_PLAY_DISCONNECT_SAME_ACCOUNT)
						{
							final Player player = World.getPlayer(charInfopackage.getObjectId());
							if ((player != null) && player.getOfflineMode().isPlayRunning())
							{
								Disconnection.of(player).storeAndDelete();
							}
						}
					}
				}
			}
		}
		catch (Exception e)
		{
			PacketLogger.warning("Could not restore char info: " + e.getMessage() + " " + e);
		}
		
		return characterList;
	}
	
	/**
	 * Loads the subclass data for a specific character from the database.<br>
	 * It updates the {@code CharacterInfoHolder} with experience, SP, level, and vitality points.
	 * @param charInfopackage The holder object where the loaded data will be stored.
	 * @param objectId The unique identifier for the character.
	 * @param activeClassId The ID of the currently active subclass.
	 */
	private static void loadCharacterSubclassInfo(CharacterInfoHolder charInfopackage, int objectId, int activeClassId)
	{
		try (Connection con = DatabaseFactory.getConnection();
			PreparedStatement statement = con.prepareStatement("SELECT exp, sp, level FROM character_subclasses WHERE charId=? AND class_id=? ORDER BY charId"))
		{
			statement.setInt(1, objectId);
			statement.setInt(2, activeClassId);
			try (ResultSet charList = statement.executeQuery())
			{
				if (charList.next())
				{
					charInfopackage.setExp(charList.getLong("exp"));
					charInfopackage.setSp(charList.getInt("sp"));
					charInfopackage.setLevel(charList.getInt("level"));
				}
			}
		}
		catch (Exception e)
		{
			PacketLogger.warning("Could not restore char subclass info: " + e.getMessage() + " " + e);
		}
	}
	
	/**
	 * Converts a {@code ResultSet} into a {@code CharacterInfoHolder} object.<br>
	 * This method extracts character data from the database and populates the holder.<br>
	 * It also handles character deletion checks and subclass information loading.<br>
	 * If the character is expired, it removes them from the clan and returns {@code null}.
	 * @param chardata The {@code ResultSet} containing the raw character data.
	 * @return A populated {@code CharacterInfoHolder} object or {@code null} if deleted.
	 * @throws Exception if an error occurs.
	 */
	private static CharacterInfoHolder restoreChar(ResultSet chardata) throws Exception
	{
		final int objectId = chardata.getInt("charId");
		final String name = chardata.getString("char_name");
		
		// See if the char must be deleted.
		final long deletetime = chardata.getLong("deletetime");
		if ((deletetime > 0) && (System.currentTimeMillis() > deletetime))
		{
			final Clan clan = ClanTable.getInstance().getClan(chardata.getInt("clanid"));
			if (clan != null)
			{
				clan.removeClanMember(objectId, 0);
			}
			
			GameClient.deleteCharByObjId(objectId);
			return null;
		}
		
		final CharacterInfoHolder charInfopackage = new CharacterInfoHolder(objectId, name);
		charInfopackage.setAccessLevel(chardata.getInt("accesslevel"));
		charInfopackage.setLevel(chardata.getInt("level"));
		charInfopackage.setMaxHp(chardata.getInt("maxhp"));
		charInfopackage.setCurrentHp(chardata.getDouble("curhp"));
		charInfopackage.setMaxMp(chardata.getInt("maxmp"));
		charInfopackage.setCurrentMp(chardata.getDouble("curmp"));
		charInfopackage.setKarma(chardata.getInt("karma"));
		charInfopackage.setPkKills(chardata.getInt("pkkills"));
		charInfopackage.setPvPKills(chardata.getInt("pvpkills"));
		charInfopackage.setFace(chardata.getInt("face"));
		charInfopackage.setHairStyle(chardata.getInt("hairstyle"));
		charInfopackage.setHairColor(chardata.getInt("haircolor"));
		charInfopackage.setSex(chardata.getInt("sex"));
		charInfopackage.setExp(chardata.getLong("exp"));
		charInfopackage.setSp(chardata.getLong("sp"));
		charInfopackage.setVitalityPoints(chardata.getInt("vitality_points"));
		charInfopackage.setClanId(chardata.getInt("clanid"));
		charInfopackage.setRace(chardata.getInt("race"));
		final int baseClassId = chardata.getInt("base_class");
		final int activeClassId = chardata.getInt("classid");
		charInfopackage.setX(chardata.getInt("x"));
		charInfopackage.setY(chardata.getInt("y"));
		charInfopackage.setZ(chardata.getInt("z"));
		final int faction = chardata.getInt("faction");
		if (faction == 1)
		{
			charInfopackage.setGood();
		}
		
		if (faction == 2)
		{
			charInfopackage.setEvil();
		}
		
		if (MultilingualSupportConfig.MULTILANG_ENABLE)
		{
			String lang = chardata.getString("language");
			if (!MultilingualSupportConfig.MULTILANG_ALLOWED.contains(lang))
			{
				lang = MultilingualSupportConfig.MULTILANG_DEFAULT;
			}
			
			charInfopackage.setHtmlPrefix("data/lang/" + lang + "/");
		}
		
		// If is in subclass, load subclass exp, sp, level info.
		if (baseClassId != activeClassId)
		{
			loadCharacterSubclassInfo(charInfopackage, objectId, activeClassId);
		}
		
		charInfopackage.setClassId(activeClassId);
		
		// Get the augmentation id for equipped weapon.
		int weaponObjId = charInfopackage.getPaperdollObjectId(Inventory.PAPERDOLL_RHAND);
		if (weaponObjId < 1)
		{
			weaponObjId = charInfopackage.getPaperdollObjectId(Inventory.PAPERDOLL_RHAND);
		}
		
		if (weaponObjId > 0)
		{
			try (Connection con = DatabaseFactory.getConnection();
				PreparedStatement statement = con.prepareStatement("SELECT augAttributes FROM item_augmentations WHERE itemId=?"))
			{
				statement.setInt(1, weaponObjId);
				try (ResultSet result = statement.executeQuery())
				{
					if (result.next())
					{
						final int augment = result.getInt("augAttributes");
						charInfopackage.setAugmentationId(augment == -1 ? 0 : augment);
					}
				}
			}
			catch (Exception e)
			{
				PacketLogger.warning("Could not restore augmentation info: " + e.getMessage() + " " + e);
			}
		}
		
		// Check if the base class is set to zero and also doesn't match with the current active class, otherwise send the base class ID. This prevents chars created before base class was introduced from being displayed incorrectly.
		if ((baseClassId == 0) && (activeClassId > 0))
		{
			charInfopackage.setBaseClassId(activeClassId);
		}
		else
		{
			charInfopackage.setBaseClassId(baseClassId);
		}
		
		charInfopackage.setDeleteTimer(deletetime);
		charInfopackage.setLastAccess(chardata.getLong("lastAccess"));
		return charInfopackage;
	}
}
